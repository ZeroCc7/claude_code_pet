from progression import (
    AdventurePhase,
    AdventureTick,
    BattleResult,
    EventResult,
    GameState,
    ItemType,
    PetForm,
    QingyunEvent,
    crc32,
)


def test_new_game_has_safe_defaults():
    state = GameState()

    assert state.level == 1
    assert state.mood == 70
    assert state.stamina == 80
    assert state.coins == 30
    assert state.energy == 10
    assert state.items == [0, 0, 0, 0, 0]
    assert state.qingyun_round == 1
    assert not state.has_qingyun_sword


def test_v1_1_state_has_no_care_actions():
    state = GameState()

    assert not hasattr(state, "interact")
    assert not hasattr(state, "feed")
    assert not hasattr(state, "meditate")
    assert not hasattr(state, "meditations_used")


def test_technique_upgrade_requires_matching_tendency_and_coins():
    state = GameState(coins=49, tendencies=[0, 0, 0, 0])

    assert not state.upgrade_technique(0)
    assert state.technique_levels == [0, 0, 0, 0]

    state.coins = 50
    assert state.upgrade_technique(0)
    assert state.technique_levels == [1, 0, 0, 0]
    assert state.coins == 0

    state.coins = 90
    state.tendencies[0] = 9
    assert not state.upgrade_technique(0)
    state.tendencies[0] = 10
    assert state.upgrade_technique(0)
    assert state.technique_levels[0] == 2


def test_technique_upgrade_stops_at_level_nine_and_does_not_change_form():
    state = GameState(
        coins=9999,
        tendencies=[250, 250, 250, 250],
        technique_levels=[8, 0, 0, 0],
        level=12,
    )
    before_form = state.form

    assert state.upgrade_technique(0)
    assert state.technique_levels[0] == 9
    assert not state.upgrade_technique(0)
    assert state.technique_levels[0] == 9
    assert state.form == before_form


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


def test_start_exploration_consumes_energy():
    state = GameState(energy=5)

    assert state.start_exploration(0)
    assert state.energy == 2
    assert state.active_region == 0


def test_qingyun_adventure_starts_for_three_energy_and_advances_one_step():
    state = GameState(energy=5)

    assert state.start_qingyun_adventure()
    assert state.energy == 2
    assert state.adventure_phase == AdventurePhase.ADVANCING

    result = state.tick_qingyun_adventure(seed=0)

    assert result == AdventureTick.ADVANCED
    assert state.energy == 1
    assert state.qingyun_progress == 2


def test_qingyun_adventure_stops_at_zero_energy_and_keeps_progress():
    state = GameState(energy=4, qingyun_progress=20)
    assert state.start_qingyun_adventure()

    assert state.tick_qingyun_adventure(seed=0) == AdventureTick.ENERGY_DEPLETED
    assert state.energy == 0
    assert state.qingyun_progress == 22
    assert state.adventure_phase == AdventurePhase.IDLE


def test_result_phase_pauses_energy_consumption():
    state = GameState(
        energy=8,
        qingyun_progress=26,
        adventure_phase=AdventurePhase.RESULT,
        current_event=QingyunEvent.SPIRIT_HERB,
    )

    assert (
        state.tick_qingyun_adventure(seed=9)
        == AdventureTick.INACTIVE
    )
    assert state.energy == 8
    assert state.qingyun_progress == 26


def test_progress_checkpoint_triggers_each_qingyun_event_once():
    state = GameState(
        energy=20,
        qingyun_progress=23,
        adventure_phase=AdventurePhase.ADVANCING,
    )

    assert (
        state.tick_qingyun_adventure(seed=0)
        == AdventureTick.EVENT_TRIGGERED
    )
    assert state.current_event == QingyunEvent.SPIRIT_HERB
    assert state.adventure_phase == AdventurePhase.RESULT
    assert state.qingyun_event_mask & 0b0001


def test_collecting_spirit_herb_adds_inventory_and_resumes():
    state = GameState(
        energy=20,
        qingyun_progress=23,
        adventure_phase=AdventurePhase.ADVANCING,
    )

    state.tick_qingyun_adventure(seed=0)

    assert state.current_event_result == EventResult.ITEM_GAINED
    assert state.items[ItemType.SPIRIT_HERB] == 1
    assert state.adventure_phase == AdventurePhase.RESULT
    state.acknowledge_adventure_result()
    assert state.adventure_phase == AdventurePhase.ADVANCING


def test_wounded_cultivator_can_award_qingyun_token_without_unlocking_boss():
    state = GameState(
        level=12,
        stamina=100,
        tendencies=[0, 0, 0, 20],
        energy=20,
        qingyun_progress=63,
        qingyun_event_mask=0b0011,
        adventure_phase=AdventurePhase.ADVANCING,
    )

    state.tick_qingyun_adventure(seed=0)

    assert state.items[ItemType.QINGYUN_TOKEN] == 1
    assert not state.qingyun_boss_unlocked
    state.acknowledge_adventure_result()
    assert state.adventure_phase == AdventurePhase.ADVANCING


def test_event_item_rewards_saturate_at_uint16_max():
    state = GameState(
        items=[65535, 0, 0, 0, 0],
        energy=20,
        qingyun_progress=23,
        adventure_phase=AdventurePhase.ADVANCING,
    )

    state.tick_qingyun_adventure(seed=0)

    assert state.items[ItemType.SPIRIT_HERB] == 65535


def test_shortcut_event_reaching_progress_100_unlocks_boss():
    state = GameState(
        energy=20,
        qingyun_progress=83,
        qingyun_event_mask=0b0111,
        adventure_phase=AdventurePhase.ADVANCING,
    )

    state.tick_qingyun_adventure(seed=0)
    assert state.current_event == QingyunEvent.SHORTCUT
    assert state.qingyun_progress >= 83
    state.qingyun_progress = 100

    state.acknowledge_adventure_result()
    assert state.qingyun_boss_unlocked
    assert state.adventure_phase == AdventurePhase.BOSS_READY


def test_qingyun_wolf_requires_unlock_and_five_energy():
    locked = GameState(energy=20)
    early = GameState(energy=20, qingyun_progress=99, qingyun_boss_unlocked=True)
    ready = GameState(energy=4, qingyun_progress=100, qingyun_boss_unlocked=True)
    capable = GameState(energy=5, qingyun_progress=100, qingyun_boss_unlocked=True)

    assert not locked.start_qingyun_wolf_battle(False, False)
    assert not early.start_qingyun_wolf_battle(False, False)
    assert not ready.start_qingyun_wolf_battle(False, False)
    assert capable.start_qingyun_wolf_battle(False, False)
    assert capable.in_battle
    assert capable.boss_hp == capable.boss_max_hp


def test_auto_battle_consumes_one_energy_each_round():
    state = GameState(
        energy=6,
        qingyun_progress=100,
        qingyun_boss_unlocked=True,
    )
    assert state.start_qingyun_wolf_battle(False, False)

    result = state.tick_qingyun_wolf_battle(seed=50)

    assert state.energy == 5
    assert state.battle_round == 1
    assert result in (BattleResult.CONTINUE, BattleResult.VICTORY)


def test_zero_energy_retreats_and_resets_boss_hp():
    state = GameState(
        energy=1,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
    )
    assert not state.start_qingyun_wolf_battle(False, False)
    state.energy = 5
    assert state.start_qingyun_wolf_battle(False, False)
    state.energy = 1

    assert (
        state.tick_qingyun_wolf_battle(seed=99)
        == BattleResult.ENERGY_DEPLETED
    )
    assert not state.in_battle
    assert state.boss_hp == state.boss_max_hp
    assert state.qingyun_progress == 0
    assert state.qingyun_event_mask == 0
    assert state.qingyun_round == 1
    assert not state.qingyun_boss_unlocked
    assert state.adventure_phase == AdventurePhase.IDLE


def test_manual_retreat_resets_boss_without_other_penalty():
    state = GameState(
        energy=8,
        stamina=67,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
    )
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp -= 5

    state.retreat_qingyun_wolf()

    assert not state.in_battle
    assert state.stamina == 67
    assert state.boss_hp == state.boss_max_hp
    assert state.qingyun_progress == 0
    assert state.qingyun_event_mask == 0
    assert state.qingyun_round == 1
    assert not state.qingyun_boss_unlocked
    assert state.adventure_phase == AdventurePhase.IDLE


def test_stamina_defeat_resets_current_run_without_losing_round():
    state = GameState(
        energy=20,
        stamina=1,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
        qingyun_round=8,
    )
    assert state.start_qingyun_wolf_battle(False, False)

    assert state.tick_qingyun_wolf_battle(seed=5000) == BattleResult.DEFEAT

    assert state.stamina == 30
    assert state.qingyun_progress == 0
    assert state.qingyun_event_mask == 0
    assert state.qingyun_round == 8
    assert not state.qingyun_boss_unlocked
    assert state.adventure_phase == AdventurePhase.IDLE


def test_attack_tendency_improves_damage_with_diminishing_returns():
    low = GameState(level=8, tendencies=[20, 0, 0, 0])
    high = GameState(level=8, tendencies=[60, 0, 0, 0])

    low_damage = low.qingyun_attack_damage(seed=50)
    first_damage = high.qingyun_attack_damage(seed=50)
    assert first_damage > low_damage
    high.tendencies[0] = 100
    second_damage = high.qingyun_attack_damage(seed=50)
    assert second_damage - first_damage <= first_damage - low_damage


def test_talismans_are_consumed_at_battle_start_and_modify_one_battle():
    state = GameState(
        energy=10,
        qingyun_progress=100,
        qingyun_boss_unlocked=True,
        items=[0, 0, 1, 1, 0],
    )

    assert state.start_qingyun_wolf_battle(True, True)
    assert state.items[ItemType.ATTACK_TALISMAN] == 0
    assert state.items[ItemType.GUARD_TALISMAN] == 0
    assert state.battle_attack_talisman
    assert state.battle_guard_talisman


def test_qingyun_difficulty_scales_quickly_then_slowly_and_caps():
    assert GameState(qingyun_round=1).qingyun_boss_max_hp() == 40
    assert GameState(qingyun_round=10).qingyun_boss_max_hp() == 112
    assert GameState(qingyun_round=11).qingyun_boss_max_hp() == 114
    assert GameState(qingyun_round=50).qingyun_boss_max_hp() == 192
    assert (
        GameState(qingyun_round=100).qingyun_boss_max_hp()
        == GameState(qingyun_round=50).qingyun_boss_max_hp()
    )


def test_qingyun_event_stamina_loss_scales_with_round():
    early = GameState(
        stamina=100,
        energy=0,
        qingyun_round=1,
        current_event=QingyunEvent.SHORTCUT,
    )
    later = GameState(
        stamina=100,
        energy=0,
        qingyun_round=10,
        current_event=QingyunEvent.SHORTCUT,
    )

    early._auto_resolve_event(seed=0)
    later._auto_resolve_event(seed=0)

    assert early.stamina == 90
    assert later.stamina == 80


def test_qingyun_completion_experience_is_small_and_capped():
    rewards = [
        GameState(qingyun_round=round_number).qingyun_completion_experience()
        for round_number in (1, 10, 20, 50, 100)
    ]

    assert rewards == [7, 16, 18, 24, 24]


def test_qingyun_victory_advances_round_and_grants_distinct_item_rewards():
    state = GameState(
        level=30,
        energy=20,
        stamina=100,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
    )
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp = 1

    assert state.tick_qingyun_wolf_battle(seed=51) == BattleResult.VICTORY

    assert state.qingyun_round == 2
    assert state.qingyun_progress == 0
    assert state.qingyun_event_mask == 0
    assert not state.qingyun_boss_unlocked
    assert state.adventure_phase == AdventurePhase.IDLE
    assert state.last_qingyun_experience == 7
    assert state.last_qingyun_coins == 17
    rewarded = [
        quantity
        for quantity in state.last_qingyun_items
        if quantity > 0
    ]
    assert rewarded == [1, 1]


def test_qingyun_sword_drops_on_twentieth_miss_and_only_once():
    state = GameState(
        level=30,
        energy=20,
        stamina=100,
        qingyun_progress=100,
        qingyun_boss_unlocked=True,
        qingyun_misses=19,
    )
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp = 1

    assert state.tick_qingyun_wolf_battle(seed=99) == BattleResult.VICTORY
    assert state.has_qingyun_sword
    assert state.last_qingyun_sword
    assert state.qingyun_misses == 0
    assert not state.qingyun_boss_unlocked

    state.energy = 20
    state.qingyun_progress = 100
    state.qingyun_boss_unlocked = True
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp = 1
    assert state.tick_qingyun_wolf_battle(seed=0) == BattleResult.VICTORY
    assert not state.last_qingyun_sword
    assert state.qingyun_misses == 0


def test_boss_requires_new_full_progress_after_battle_end():
    state = GameState(
        level=30,
        energy=20,
        stamina=100,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
    )
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp = 1
    assert state.tick_qingyun_wolf_battle(seed=51) == BattleResult.VICTORY

    state.energy = 15
    assert not state.start_qingyun_wolf_battle(False, False)
    assert state.qingyun_progress == 0
    assert not state.qingyun_boss_unlocked


def test_starting_adventure_from_boss_ready_does_not_reset_pending_boss():
    state = GameState(
        energy=20,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
        adventure_phase=AdventurePhase.BOSS_READY,
    )
    assert not state.start_qingyun_adventure()
    assert state.energy == 20
    assert state.qingyun_progress == 100
    assert state.qingyun_event_mask == 0b1111
    assert state.adventure_phase == AdventurePhase.BOSS_READY


def test_qingyun_sword_increases_damage_and_reduces_incoming_damage():
    ordinary = GameState(level=8)
    sword = GameState(level=8, has_qingyun_sword=True)

    assert sword.qingyun_attack_damage(seed=50) == (
        ordinary.qingyun_attack_damage(seed=50) * 110 // 100
    )
    assert sword.qingyun_incoming_damage(seed=5000) == max(
        1, ordinary.qingyun_incoming_damage(seed=5000) * 90 // 100
    )


def test_failed_task_grants_reduced_experience():
    state = GameState()

    state.apply_task("codex", duration_seconds=300, success=False)

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
    state = GameState(level=11, experience=489, form=PetForm.ROOKIE_B)
    state.tendencies = [1, 2, 12, 6]

    state.gain_experience(1)

    assert state.level == 12
    assert state.form == PetForm.FINAL_B1


def test_runtime_recovers_one_energy_every_five_minutes():
    state = GameState(energy=10, stamina=100)

    assert not state.tick_runtime(299)
    assert state.energy == 10
    assert state.tick_runtime(1)
    assert state.energy == 11


def test_passive_recovery_never_exceeds_twenty_or_banks_ticks():
    state = GameState(energy=20, stamina=100)

    assert not state.tick_runtime(900)
    assert state.energy == 20
    state.energy = 19
    assert not state.tick_runtime(299)
    assert state.tick_runtime(1)
    assert state.energy == 20


def test_runtime_recovers_five_stamina_every_five_minutes():
    state = GameState(energy=20, stamina=84)

    assert not state.tick_runtime(299)
    assert state.stamina == 84
    assert state.tick_runtime(1)
    assert state.stamina == 89


def test_stamina_recovery_caps_at_one_hundred_without_banking_time():
    state = GameState(energy=20, stamina=98)

    assert state.tick_runtime(300)
    assert state.stamina == 100
    assert state.stamina_recovery_seconds == 0
    assert not state.tick_runtime(900)
    state.stamina = 95
    assert not state.tick_runtime(299)
    assert state.stamina == 95
    assert state.tick_runtime(1)
    assert state.stamina == 100


def test_task_rewards_respect_energy_cap():
    state = GameState(energy=19)

    state.apply_task("codex", duration_seconds=600, success=True)

    assert state.energy == 20


def test_completed_ai_task_grants_rewards():
    state = GameState()

    state.complete_ai_task("codex", 300)

    assert (state.experience, state.coins, state.energy) == (10, 35, 12)


def test_completed_ai_task_records_source_duration_and_rewards():
    state = GameState()

    state.complete_ai_task("codex", 300)

    records = state.recent_ai_tasks()
    assert len(records) == 1
    assert records[0].source == "codex"
    assert records[0].duration_seconds == 300
    assert records[0].experience_reward == 10
    assert records[0].coin_reward == 5


def test_recent_ai_task_log_keeps_ten_newest_records():
    state = GameState()

    for index in range(11):
        state.complete_ai_task("codex", (index + 1) * 60)

    records = state.recent_ai_tasks()
    assert len(records) == 10
    assert records[0].duration_seconds == 660
    assert records[-1].duration_seconds == 120


def test_task_records_preserve_source():
    state = GameState()

    state.complete_ai_task("codex", 60)
    state.complete_ai_task("codefree-o", 60)

    assert state.experience == 4
    assert [record.source for record in state.recent_ai_tasks()] == [
        "codefree-o",
        "codex",
    ]


def test_halved_task_grants_half_experience_and_coins():
    state = GameState()

    state.apply_task("codex", duration_seconds=600, success=True, halved=True)

    assert state.experience == 10
    assert state.coins == 35


def test_halved_ai_task_grants_half_rewards():
    state = GameState()

    state.complete_ai_task("codex", 600, halved=True)

    assert state.experience == 10
    assert state.coins == 35


def test_halved_task_minimum_reward_is_one():
    state = GameState()

    state.apply_task("codex", duration_seconds=60, success=True, halved=True)

    assert state.experience == 1
    assert state.coins == 31


def test_auto_resolve_events_grow_tendencies():
    state = GameState(
        level=20,
        stamina=100,
        energy=20,
        current_event=QingyunEvent.DEMON_BEAST,
    )
    state._auto_resolve_event(seed=0)
    assert state.tendencies[0] == 2

    state2 = GameState(
        level=20,
        stamina=100,
        energy=20,
        current_event=QingyunEvent.SHORTCUT,
    )
    state2._auto_resolve_event(seed=0)
    assert state2.tendencies[1] == 2

    state3 = GameState(
        level=20,
        stamina=100,
        energy=20,
        tendencies=[0, 0, 0, 20],
        current_event=QingyunEvent.WOUNDED_CULTIVATOR,
    )
    state3._auto_resolve_event(seed=0)
    assert state3.tendencies[3] == 22

    state4 = GameState(
        energy=20,
        current_event=QingyunEvent.SPIRIT_HERB,
    )
    state4._auto_resolve_event(seed=0)
    assert state4.tendencies[3] == 1

    state5 = GameState(
        level=1,
        stamina=10,
        current_event=QingyunEvent.DEMON_BEAST,
    )
    state5._auto_resolve_event(seed=0)
    assert state5.tendencies == [0, 0, 0, 0]


def test_ai_task_grows_tendency_by_source():
    codex = GameState()
    codex.apply_task("codex", 600, True)
    assert codex.tendencies[0] == 2

    claude = GameState()
    claude.apply_task("claude-code", 600, True)
    assert claude.tendencies[1] == 2

    opencode = GameState()
    opencode.apply_task("opencode", 600, True)
    assert opencode.tendencies[2] == 2

    other = GameState()
    other.apply_task("codefree-o", 600, True)
    assert other.tendencies[3] == 2


def test_ai_task_tendency_gain_scales_with_duration():
    short = GameState()
    short.apply_task("codex", 60, True)
    assert short.tendencies[0] == 1

    medium = GameState()
    medium.apply_task("codex", 600, True)
    assert medium.tendencies[0] == 2

    long = GameState()
    long.apply_task("codex", 1800, True)
    assert long.tendencies[0] == 4


def test_failed_ai_task_gives_no_tendency():
    state = GameState()
    state.apply_task("codex", 600, False)
    assert state.tendencies == [0, 0, 0, 0]


def test_boss_victory_grows_all_tendencies():
    state = GameState(
        level=30,
        energy=20,
        stamina=100,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
    )
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp = 1

    state.tick_qingyun_wolf_battle(seed=51)

    assert state.tendencies[0] == 2
    assert state.tendencies[1] == 2
    assert state.tendencies[2] == 2
    assert state.tendencies[3] == 2


def test_boss_victory_tendency_bonus_scales_with_round():
    state = GameState(
        level=30,
        energy=20,
        stamina=100,
        qingyun_progress=100,
        qingyun_event_mask=0b1111,
        qingyun_boss_unlocked=True,
        qingyun_round=25,
    )
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp = 1

    state.tick_qingyun_wolf_battle(seed=51)

    assert state.tendencies[0] == 5
    assert state.tendencies[1] == 5
    assert state.tendencies[2] == 5
    assert state.tendencies[3] == 5


def test_tendency_caps_at_one_hundred():
    state = GameState(tendencies=[98, 99, 100, 50])

    state._add_tendency(0, 5)
    state._add_tendency(1, 5)
    state._add_tendency(2, 5)
    state._add_tendency(3, 5)

    assert state.tendencies == [100, 100, 100, 55]


def test_complete_ai_task_grows_tendency():
    state = GameState()

    state.complete_ai_task("codex", 600)

    assert state.tendencies[0] == 2
    assert state.tendencies[1] == 0
    assert state.tendencies[2] == 0
    assert state.tendencies[3] == 0


def test_all_four_events_fire_exactly_once_per_run():
    state = GameState(energy=200, level=10, stamina=100)
    assert state.start_qingyun_adventure()

    fired_events = []
    for _ in range(60):
        tick = state.tick_qingyun_adventure(seed=42)
        if tick == AdventureTick.EVENT_TRIGGERED:
            fired_events.append(state.current_event)
            state.acknowledge_adventure_result()
        elif tick == AdventureTick.BOSS_UNLOCKED:
            break

    assert len(fired_events) == 4
    assert set(fired_events) == {
        QingyunEvent.SPIRIT_HERB,
        QingyunEvent.DEMON_BEAST,
        QingyunEvent.WOUNDED_CULTIVATOR,
        QingyunEvent.SHORTCUT,
    }


def test_shuffled_event_order_varies_between_adventures():
    orders = set()
    for _ in range(20):
        state = GameState(energy=10)
        state.start_qingyun_adventure()
        orders.add(state.qingyun_event_order)

    # With 20 attempts and 24 possible permutations, we should see at least 2
    assert len(orders) >= 2
