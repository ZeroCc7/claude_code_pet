from progression import GameState, ItemType, PetForm, crc32


def test_new_game_has_safe_defaults():
    state = GameState()

    assert state.level == 1
    assert state.mood == 70
    assert state.stamina == 80
    assert state.coins == 30
    assert state.energy == 10
    assert state.items == [0, 0, 0, 0, 0]


def test_spirit_herb_restores_three_energy_and_consumes_one():
    state = GameState(energy=16, items=[1, 0, 0, 0, 0])

    assert state.use_item(ItemType.SPIRIT_HERB)
    assert state.energy == 19
    assert state.items[ItemType.SPIRIT_HERB] == 0


def test_spirit_herb_respects_cap_and_is_not_wasted_when_full():
    state = GameState(energy=19, items=[2, 0, 0, 0, 0])

    assert state.use_item(ItemType.SPIRIT_HERB)
    assert state.energy == 20
    assert state.items[ItemType.SPIRIT_HERB] == 1
    assert not state.use_item(ItemType.SPIRIT_HERB)
    assert state.items[ItemType.SPIRIT_HERB] == 1


def test_recovery_pill_restores_twenty_stamina_and_consumes_one():
    state = GameState(stamina=75, items=[0, 1, 0, 0, 0])

    assert state.use_item(ItemType.RECOVERY_PILL)
    assert state.stamina == 95
    assert state.items[ItemType.RECOVERY_PILL] == 0


def test_recovery_item_fails_without_stock():
    state = GameState(energy=10, stamina=50)

    assert not state.use_item(ItemType.SPIRIT_HERB)
    assert not state.use_item(ItemType.RECOVERY_PILL)


def test_interaction_improves_mood_without_exceeding_100():
    state = GameState(mood=98)

    state.interact()

    assert state.mood == 100


def test_feed_spends_coins_and_restores_stamina():
    state = GameState(coins=20, stamina=70)

    assert state.feed()
    assert state.coins == 10
    assert state.stamina == 90


def test_feed_fails_without_enough_coins():
    state = GameState(coins=9, stamina=50)

    assert not state.feed()
    assert state.coins == 9
    assert state.stamina == 50


def test_start_exploration_consumes_energy():
    state = GameState(energy=5)

    assert state.start_exploration(0)
    assert state.energy == 2
    assert state.active_region == 0


def test_failed_task_grants_reduced_experience():
    state = GameState()

    state.apply_task(duration_seconds=300, success=False)

    assert state.experience == 3
    assert state.coins == 30
    assert state.energy == 10


def test_crc32_matches_standard_vector():
    assert crc32(b"123456789") == 0xCBF43926


def test_exploration_tick_consumes_energy_and_advances_region():
    state = GameState(energy=5)
    assert state.start_exploration(0)

    state.tick_exploration(seed=1)

    assert state.energy == 1
    assert state.region_progress[0] == 2
    assert state.coins == 31


def test_exploration_grants_experience_by_region():
    rewards = []
    for region in range(3):
        state = GameState(
            energy=10,
            boss_defeated_mask=(1 << region) - 1,
        )
        assert state.start_exploration(region)

        state.tick_exploration(seed=0)

        rewards.append(state.experience)

    assert rewards == [1, 2, 3]


def test_locked_region_cannot_start_until_previous_boss_is_defeated():
    state = GameState(energy=10)

    assert not state.start_exploration(1)
    state.boss_defeated_mask = 0b001
    assert state.start_exploration(1)


def test_boss_battle_attack_and_defense():
    state = GameState(region_progress=[100, 0, 0], stamina=80)
    assert state.start_boss(0)

    first_hp = state.boss_hp
    state.battle_action("attack")
    assert state.boss_hp < first_hp
    stamina_after_attack = state.stamina

    state.battle_action("defend")
    assert state.stamina >= stamina_after_attack - 3


def test_boss_victory_unlocks_next_region():
    state = GameState(region_progress=[100, 0, 0], stamina=100)
    assert state.start_boss(0)
    state.boss_hp = 1

    state.battle_action("attack")

    assert state.boss_defeated_mask & 0b001
    assert state.coins >= 50
    assert not state.in_battle


def test_level_three_evolves_to_a_rookie_branch():
    state = GameState(level=2, experience=39)
    state.tendencies = [8, 2, 1, 0]

    state.gain_experience(1)

    assert state.level == 3
    assert state.form == PetForm.ROOKIE_A


def test_defeated_boss_can_be_rechallenged_with_halved_rewards():
    state = GameState(
        level=10,
        experience=180,
        stamina=100,
        region_progress=[100, 0, 0],
    )
    rewards = []

    for _ in range(3):
        assert state.start_boss(0)
        before_xp = state.experience
        before_coins = state.coins
        state.boss_hp = 1
        state.battle_action("attack")
        rewards.append(
            (state.experience - before_xp, state.coins - before_coins)
        )

    assert rewards == [(20, 20), (10, 10), (5, 5)]
    assert state.boss_wins == [3, 0, 0]


def test_repeated_boss_reward_never_falls_below_one():
    state = GameState(
        stamina=100,
        region_progress=[100, 0, 0],
        boss_defeated_mask=0b001,
        boss_wins=[20, 0, 0],
    )

    assert state.start_boss(0)
    before_xp = state.experience
    before_coins = state.coins
    state.boss_hp = 1
    state.battle_action("attack")

    assert state.experience - before_xp == 1
    assert state.coins - before_coins == 1


def test_level_twelve_evolves_to_matching_final_form():
    state = GameState(level=11, experience=219, form=PetForm.ROOKIE_B)
    state.tendencies = [1, 2, 12, 6]

    state.gain_experience(1)

    assert state.level == 12
    assert state.form == PetForm.FINAL_B1


def test_runtime_recovers_one_energy_every_five_minutes():
    state = GameState(energy=10)

    assert not state.tick_runtime(299)
    assert state.energy == 10
    assert state.tick_runtime(1)
    assert state.energy == 11


def test_passive_recovery_never_exceeds_twenty_or_banks_ticks():
    state = GameState(energy=20)

    assert not state.tick_runtime(900)
    assert state.energy == 20
    state.energy = 19
    assert not state.tick_runtime(299)
    assert state.tick_runtime(1)
    assert state.energy == 20


def test_meditation_restores_energy_and_consumes_one_use():
    state = GameState(energy=12)

    assert state.meditate() == "restored"
    assert state.energy == 15
    assert state.meditations_used == 1


def test_full_energy_meditation_does_not_consume_use():
    state = GameState(energy=20)

    assert state.meditate() == "full"
    assert state.meditations_used == 0


def test_fourth_meditation_fails_until_runtime_cycle_resets():
    state = GameState(energy=5, meditations_used=3)

    assert state.meditate() == "exhausted"
    state.tick_runtime(86400)
    assert state.meditations_used == 0
    state.energy = 10
    assert state.meditate() == "restored"


def test_task_rewards_respect_energy_cap():
    state = GameState(energy=19)

    state.apply_task(duration_seconds=600, success=True)

    assert state.energy == 20


def test_completed_ai_task_is_rewarded_only_once():
    state = GameState()

    assert state.apply_ai_task("codex", "task-123", 300, True)
    first_reward = (state.experience, state.coins, state.energy)
    assert not state.apply_ai_task("codex", "task-123", 300, True)

    assert (state.experience, state.coins, state.energy) == first_reward


def test_task_identity_includes_source():
    state = GameState()

    assert state.apply_ai_task("codex", "same-id", 60, True)
    assert state.apply_ai_task("claude_code", "same-id", 60, True)

    assert state.experience == 4


def test_recent_task_ring_replaces_oldest_after_sixteen_entries():
    state = GameState()
    for index in range(17):
        assert state.apply_ai_task("codex", f"task-{index}", 60, True)

    assert state.apply_ai_task("codex", "task-0", 60, True)
    assert not state.apply_ai_task("codex", "task-16", 60, True)


def test_halved_task_grants_half_experience_and_coins():
    state = GameState()

    state.apply_task(duration_seconds=600, success=True, halved=True)

    assert state.experience == 10
    assert state.coins == 35


def test_halved_ai_task_grants_half_rewards():
    state = GameState()

    assert state.apply_ai_task("codex", "timeout-1", 600, True, halved=True)

    assert state.experience == 10
    assert state.coins == 35


def test_halved_task_minimum_reward_is_one():
    state = GameState()

    state.apply_task(duration_seconds=60, success=True, halved=True)

    assert state.experience == 1
    assert state.coins == 31
