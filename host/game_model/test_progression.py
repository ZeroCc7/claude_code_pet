from progression import GameState, crc32


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
