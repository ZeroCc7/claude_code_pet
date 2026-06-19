from progression import GameState, PetForm, crc32


def test_new_game_has_safe_defaults():
    state = GameState()

    assert state.level == 1
    assert state.mood == 70
    assert state.stamina == 80
    assert state.coins == 30
    assert state.energy == 10


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


def test_level_five_evolves_to_a_rookie_branch():
    state = GameState(level=4, experience=79)
    state.tendencies = [8, 2, 1, 0]

    state.gain_experience(1)

    assert state.level == 5
    assert state.form == PetForm.ROOKIE_A


def test_level_twelve_evolves_to_matching_final_form():
    state = GameState(level=11, experience=219, form=PetForm.ROOKIE_B)
    state.tendencies = [1, 2, 12, 6]

    state.gain_experience(1)

    assert state.level == 12
    assert state.form == PetForm.FINAL_B1
