from __future__ import annotations

from dataclasses import dataclass, field
from enum import IntEnum


class PetForm(IntEnum):
    EGG = 0
    ROOKIE_A = 1
    ROOKIE_B = 2
    FINAL_A1 = 3
    FINAL_A2 = 4
    FINAL_B1 = 5
    FINAL_B2 = 6


class ItemType(IntEnum):
    SPIRIT_HERB = 0
    RECOVERY_PILL = 1
    ATTACK_TALISMAN = 2
    GUARD_TALISMAN = 3
    QINGYUN_TOKEN = 4


class AdventurePhase(IntEnum):
    IDLE = 0
    ADVANCING = 1
    CHOOSING = 2
    RESULT = 3
    BOSS_READY = 4


class AdventureTick(IntEnum):
    INACTIVE = 0
    ADVANCED = 1
    EVENT_TRIGGERED = 2
    WAITING_FOR_CHOICE = 3
    ENERGY_DEPLETED = 4
    BOSS_UNLOCKED = 5


class QingyunEvent(IntEnum):
    NONE = 0
    SPIRIT_HERB = 1
    DEMON_BEAST = 2
    WOUNDED_CULTIVATOR = 3
    SHORTCUT = 4


class EventResult(IntEnum):
    NONE = 0
    CONTINUED = 1
    ITEM_GAINED = 2
    REWARD_GAINED = 3
    PROGRESS_GAINED = 4
    STAMINA_LOST = 5


class BattleResult(IntEnum):
    INACTIVE = 0
    CONTINUE = 1
    VICTORY = 2
    DEFEAT = 3
    ENERGY_DEPLETED = 4
    RETREATED = 5


@dataclass(frozen=True)
class AiTaskRecord:
    source: str
    duration_seconds: int
    experience_reward: int
    coin_reward: int


def crc32(payload: bytes) -> int:
    value = 0xFFFFFFFF
    for byte in payload:
        value ^= byte
        for _ in range(8):
            mask = -(value & 1) & 0xFFFFFFFF
            value = (value >> 1) ^ (0xEDB88320 & mask)
    return value ^ 0xFFFFFFFF


@dataclass
class GameState:
    level: int = 1
    experience: int = 0
    mood: int = 70
    stamina: int = 80
    coins: int = 30
    energy: int = 10
    active_region: int | None = None
    region_progress: list[int] = field(default_factory=lambda: [0, 0, 0])
    boss_defeated_mask: int = 0
    boss_wins: list[int] = field(default_factory=lambda: [0, 0, 0])
    tendencies: list[int] = field(default_factory=lambda: [0, 0, 0, 0])
    form: PetForm = PetForm.EGG
    in_battle: bool = False
    battle_region: int = 0
    boss_hp: int = 0
    boss_max_hp: int = 0
    energy_recovery_seconds: int = 0
    items: list[int] = field(default_factory=lambda: [0] * 5)
    ai_task_records: list[AiTaskRecord | None] = field(
        default_factory=lambda: [None] * 10
    )
    ai_task_record_index: int = 0
    ai_task_record_count: int = 0
    qingyun_progress: int = 0
    adventure_phase: AdventurePhase = AdventurePhase.IDLE
    current_event: QingyunEvent = QingyunEvent.NONE
    current_event_result: EventResult = EventResult.NONE
    qingyun_event_mask: int = 0
    qingyun_boss_unlocked: bool = False
    qingyun_boss_defeated: bool = False
    qingyun_boss_wins: int = 0
    qingyun_round: int = 1
    qingyun_misses: int = 0
    has_qingyun_sword: bool = False
    stamina_recovery_seconds: int = 0
    last_qingyun_experience: int = 0
    last_qingyun_coins: int = 0
    last_qingyun_items: list[int] = field(default_factory=lambda: [0] * 4)
    last_qingyun_sword: bool = False
    battle_round: int = 0
    battle_attack_talisman: bool = False
    battle_guard_talisman: bool = False
    last_battle_result: BattleResult = BattleResult.INACTIVE

    def max_energy(self) -> int:
        if self.form >= PetForm.FINAL_A1:
            return 80
        if self.form >= PetForm.ROOKIE_A:
            return 40
        return 20

    def use_item(self, item: ItemType) -> bool:
        if self.items[item] == 0:
            return False
        if item == ItemType.SPIRIT_HERB:
            if self.energy >= self.max_energy():
                return False
            self.energy = min(self.max_energy(), self.energy + 3)
        elif item == ItemType.RECOVERY_PILL:
            if self.stamina >= 100:
                return False
            self.stamina = min(100, self.stamina + 20)
        else:
            return False
        self.items[item] -= 1
        return True

    def tick_runtime(self, seconds: int) -> bool:
        changed = False
        if self.energy >= self.max_energy():
            self.energy_recovery_seconds = 0
        else:
            self.energy_recovery_seconds += seconds
            while self.energy_recovery_seconds >= 300 and self.energy < self.max_energy():
                self.energy_recovery_seconds -= 300
                self.energy += 1
                changed = True
            if self.energy >= self.max_energy():
                self.energy_recovery_seconds = 0

        if self.stamina >= 100:
            self.stamina_recovery_seconds = 0
        else:
            self.stamina_recovery_seconds += seconds
            while (
                self.stamina_recovery_seconds >= 300
                and self.stamina < 100
            ):
                self.stamina_recovery_seconds -= 300
                self.stamina = min(100, self.stamina + 5)
                changed = True
            if self.stamina >= 100:
                self.stamina_recovery_seconds = 0
        return changed

    def region_unlocked(self, region: int) -> bool:
        return region == 0 or bool(self.boss_defeated_mask & (1 << (region - 1)))

    def start_exploration(self, region: int) -> bool:
        if (
            region not in (0, 1, 2)
            or not self.region_unlocked(region)
            or self.energy < 3
        ):
            return False
        self.energy -= 3
        self.active_region = region
        return True

    def start_qingyun_adventure(self) -> bool:
        if self.adventure_phase == AdventurePhase.BOSS_READY:
            return False
        if self.adventure_phase != AdventurePhase.IDLE:
            return False
        if self.energy < 3:
            return False
        self.energy -= 3
        self.adventure_phase = AdventurePhase.ADVANCING
        self.current_event = QingyunEvent.NONE
        return True

    def stop_qingyun_adventure(self) -> None:
        self.adventure_phase = (
            AdventurePhase.BOSS_READY
            if self.qingyun_progress >= 100
            else AdventurePhase.IDLE
        )
        self.current_event = QingyunEvent.NONE

    def tick_qingyun_adventure(self, seed: int) -> AdventureTick:
        if self.adventure_phase == AdventurePhase.CHOOSING:
            return AdventureTick.WAITING_FOR_CHOICE
        if self.adventure_phase != AdventurePhase.ADVANCING:
            return AdventureTick.INACTIVE
        self.energy -= 1
        self.qingyun_progress = min(100, self.qingyun_progress + 2)
        checkpoints = (25, 45, 65, 85)
        events = (
            QingyunEvent.SPIRIT_HERB,
            QingyunEvent.DEMON_BEAST,
            QingyunEvent.WOUNDED_CULTIVATOR,
            QingyunEvent.SHORTCUT,
        )
        for index, checkpoint in enumerate(checkpoints):
            event_bit = 1 << index
            if (
                self.qingyun_progress >= checkpoint
                and not self.qingyun_event_mask & event_bit
            ):
                self.qingyun_event_mask |= event_bit
                self.current_event = events[index]
                self.current_event_result = EventResult.NONE
                self.adventure_phase = AdventurePhase.CHOOSING
                return AdventureTick.EVENT_TRIGGERED
        if self.qingyun_progress >= 100:
            self.qingyun_boss_unlocked = True
            self.adventure_phase = AdventurePhase.BOSS_READY
            return AdventureTick.BOSS_UNLOCKED
        if self.energy == 0:
            self.stop_qingyun_adventure()
            return AdventureTick.ENERGY_DEPLETED
        return AdventureTick.ADVANCED

    def resolve_qingyun_event(self, choice: int, seed: int) -> EventResult:
        if self.adventure_phase != AdventurePhase.CHOOSING:
            return EventResult.NONE
        result = EventResult.CONTINUED
        if self.current_event == QingyunEvent.SPIRIT_HERB and choice == 0:
            self._add_item(ItemType.SPIRIT_HERB)
            result = EventResult.ITEM_GAINED
        elif self.current_event == QingyunEvent.DEMON_BEAST and choice == 0:
            score = (
                self.level
                + self.stamina // 10
                + self.tendencies[0] // 4
                + seed % 10
            )
            if score >= 18:
                self.gain_experience(6)
                self.coins += 5
                result = EventResult.REWARD_GAINED
            else:
                self.stamina = max(
                    0, self.stamina - self._qingyun_event_damage(12)
                )
                result = EventResult.STAMINA_LOST
        elif (
            self.current_event == QingyunEvent.WOUNDED_CULTIVATOR
            and choice == 0
        ):
            score = (
                self.level
                + self.stamina // 10
                + self.tendencies[3] // 2
                + seed % 10
            )
            if score >= 25:
                self._add_item(ItemType.QINGYUN_TOKEN)
            else:
                self._add_item(ItemType.RECOVERY_PILL)
            result = EventResult.ITEM_GAINED
        elif self.current_event == QingyunEvent.SHORTCUT:
            if choice == 0:
                score = (
                    self.level
                    + self.energy
                    + self.tendencies[1] // 3
                    + seed % 10
                )
                if score >= 20:
                    self.qingyun_progress = min(
                        100, self.qingyun_progress + 8 + seed % 8
                    )
                    result = EventResult.PROGRESS_GAINED
                else:
                    self.stamina = max(
                        0, self.stamina - self._qingyun_event_damage(10)
                    )
                    result = EventResult.STAMINA_LOST
            else:
                self.qingyun_progress = min(100, self.qingyun_progress + 3)
                result = EventResult.PROGRESS_GAINED
        self.current_event_result = result
        self.adventure_phase = AdventurePhase.RESULT
        return result

    def acknowledge_adventure_result(self) -> None:
        if self.adventure_phase != AdventurePhase.RESULT:
            return
        self.current_event = QingyunEvent.NONE
        self.current_event_result = EventResult.NONE
        if self.qingyun_progress >= 100:
            self.qingyun_boss_unlocked = True
        self.adventure_phase = (
            AdventurePhase.BOSS_READY
            if self.qingyun_progress >= 100
            else AdventurePhase.ADVANCING
        )

    def abandon_qingyun_event(self) -> None:
        if self.adventure_phase != AdventurePhase.CHOOSING:
            return
        self.stop_qingyun_adventure()

    def _add_item(self, item: ItemType) -> None:
        self.items[item] = min(65535, self.items[item] + 1)

    def start_qingyun_wolf_battle(
        self,
        use_attack_talisman: bool,
        use_guard_talisman: bool,
    ) -> bool:
        if (
            self.in_battle
            or not self.qingyun_boss_unlocked
            or self.qingyun_progress < 100
            or self.energy < 5
        ):
            return False
        self.in_battle = True
        self.boss_max_hp = self.qingyun_boss_max_hp()
        self.boss_hp = self.boss_max_hp
        self.battle_round = 0
        self.last_battle_result = BattleResult.CONTINUE
        self.battle_attack_talisman = (
            use_attack_talisman
            and self.items[ItemType.ATTACK_TALISMAN] > 0
        )
        self.battle_guard_talisman = (
            use_guard_talisman
            and self.items[ItemType.GUARD_TALISMAN] > 0
        )
        if self.battle_attack_talisman:
            self.items[ItemType.ATTACK_TALISMAN] -= 1
        if self.battle_guard_talisman:
            self.items[ItemType.GUARD_TALISMAN] -= 1
        return True

    def qingyun_attack_damage(self, seed: int) -> int:
        damage = (
            4
            + self.level
            + min(8, self.tendencies[0] // 5)
            + min(10, self.tendencies[1] // 4)
        )
        if self.form in (PetForm.ROOKIE_A, PetForm.FINAL_A1):
            damage += 1
        elif self.form == PetForm.FINAL_A2:
            damage += 2
        critical_rate = 5 + min(15, self.tendencies[0] // 4)
        if seed % 100 < critical_rate:
            damage *= 2
        if self.battle_attack_talisman:
            damage = damage * 120 // 100
        if self.has_qingyun_sword:
            damage = damage * 110 // 100
        return max(1, damage)

    def qingyun_incoming_damage(self, seed: int) -> int:
        dodge_rate = 5 + min(15, self.tendencies[2] // 4)
        if (seed // 100) % 100 < dodge_rate:
            return 0
        damage = max(1, 8 - min(5, self.tendencies[2] // 8))
        if self.form in (PetForm.ROOKIE_B, PetForm.FINAL_B1):
            damage = max(1, damage - 1)
        elif self.form == PetForm.FINAL_B2:
            damage = max(1, damage - 2)
        if self.battle_guard_talisman:
            damage = max(1, damage * 80 // 100)
        damage = max(1, damage * self._qingyun_damage_percent() // 100)
        if self.has_qingyun_sword:
            damage = max(1, damage * 90 // 100)
        return damage

    def tick_qingyun_wolf_battle(self, seed: int) -> BattleResult:
        if not self.in_battle:
            return BattleResult.INACTIVE
        self.energy -= 1
        self.battle_round += 1
        if self.energy == 0:
            self._reset_qingyun_run()
            self._finish_qingyun_battle(BattleResult.ENERGY_DEPLETED)
            return BattleResult.ENERGY_DEPLETED

        self.boss_hp = max(
            0, self.boss_hp - self.qingyun_attack_damage(seed)
        )
        if self.boss_hp == 0:
            experience_reward = self.qingyun_completion_experience()
            coin_reward = self.qingyun_completion_coins()
            self.last_qingyun_experience = experience_reward
            self.last_qingyun_coins = coin_reward
            self._grant_qingyun_items(seed)
            self._roll_qingyun_sword(seed)
            self.qingyun_boss_wins += 1
            self.qingyun_boss_defeated = True
            self.gain_experience(experience_reward)
            self.coins = min(65535, self.coins + coin_reward)
            self.qingyun_round = min(65535, self.qingyun_round + 1)
            self._reset_qingyun_run()
            self._finish_qingyun_battle(BattleResult.VICTORY, reset_hp=False)
            return BattleResult.VICTORY

        incoming = self.qingyun_incoming_damage(seed)
        self.stamina = max(0, self.stamina - incoming)
        affinity_rate = min(20, self.tendencies[3] // 3)
        if (
            self.stamina > 0
            and (seed // 10000) % 100 < affinity_rate
        ):
            self.stamina = min(100, self.stamina + 2)
        if self.stamina == 0:
            self.stamina = 30
            self._reset_qingyun_run()
            self._finish_qingyun_battle(BattleResult.DEFEAT)
            return BattleResult.DEFEAT
        self.last_battle_result = BattleResult.CONTINUE
        return BattleResult.CONTINUE

    def retreat_qingyun_wolf(self) -> None:
        if not self.in_battle:
            return
        self._reset_qingyun_run()
        self._finish_qingyun_battle(BattleResult.RETREATED)

    def qingyun_boss_max_hp(self) -> int:
        return 48 * self._qingyun_health_percent() // 100

    def qingyun_completion_experience(self) -> int:
        round_number = min(50, max(1, self.qingyun_round))
        return (
            6
            + min(round_number, 10)
            + max(0, round_number - 10) // 5
        )

    def qingyun_completion_coins(self) -> int:
        round_number = min(50, max(1, self.qingyun_round))
        return (
            15
            + 2 * min(round_number, 10)
            + max(0, round_number - 10)
        )

    def _qingyun_health_percent(self) -> int:
        round_number = min(50, max(1, self.qingyun_round))
        return (
            100
            + min(round_number - 1, 9) * 15
            + max(0, round_number - 10) * 3
        )

    def _qingyun_damage_percent(self) -> int:
        round_number = min(50, max(1, self.qingyun_round))
        return (
            100
            + min(round_number - 1, 9) * 8
            + max(0, round_number - 10) * 2
        )

    def _qingyun_event_damage(self, base_damage: int) -> int:
        return max(1, base_damage * self._qingyun_damage_percent() // 100)

    def _grant_qingyun_items(self, seed: int) -> None:
        self.last_qingyun_items = [0] * 4
        reward_count = 1 + seed % 2
        quantity = 1 + min(4, (max(1, self.qingyun_round) - 1) // 10)
        first = (seed // 2) % 4
        reward_types = [first]
        if reward_count == 2:
            reward_types.append((first + 1 + (seed // 8) % 3) % 4)
        for item in reward_types:
            self.last_qingyun_items[item] = quantity
            self.items[item] = min(65535, self.items[item] + quantity)

    def _roll_qingyun_sword(self, seed: int) -> None:
        self.last_qingyun_sword = False
        if self.has_qingyun_sword:
            self.qingyun_misses = 0
            return
        chance = min(10, max(1, self.qingyun_round))
        if self.qingyun_misses >= 19 or seed % 100 < chance:
            self.has_qingyun_sword = True
            self.last_qingyun_sword = True
            self.qingyun_misses = 0
        else:
            self.qingyun_misses = min(255, self.qingyun_misses + 1)

    def _reset_qingyun_run(self) -> None:
        self.qingyun_progress = 0
        self.qingyun_event_mask = 0
        self.qingyun_boss_unlocked = False
        self.adventure_phase = AdventurePhase.IDLE
        self.current_event = QingyunEvent.NONE
        self.current_event_result = EventResult.NONE

    def _finish_qingyun_battle(
        self,
        result: BattleResult,
        *,
        reset_hp: bool = True,
    ) -> None:
        self.in_battle = False
        self.last_battle_result = result
        self.adventure_phase = AdventurePhase.IDLE
        if reset_hp:
            self.boss_hp = self.boss_max_hp
        self.battle_attack_talisman = False
        self.battle_guard_talisman = False

    def tick_exploration(self, seed: int) -> None:
        if self.active_region is None or self.energy == 0:
            return
        self.energy -= 1
        gain = 1 + seed % 3
        region = self.active_region
        self.region_progress[region] = min(100, self.region_progress[region] + gain)
        self.coins += 1
        self.gain_experience(region + 1)
        self.tendencies[0 if region == 0 else 2] += 1
        if self.region_progress[region] == 100 or self.energy == 0:
            self.active_region = None

    def start_boss(self, region: int) -> bool:
        if (
            region not in (0, 1, 2)
            or self.region_progress[region] < 100
        ):
            return False
        self.in_battle = True
        self.battle_region = region
        self.boss_max_hp = 25 + region * 20
        self.boss_hp = self.boss_max_hp
        return True

    def battle_action(self, action: str) -> None:
        if not self.in_battle:
            return
        damage = 0
        incoming = 5 + self.battle_region * 2
        if action == "attack":
            damage = 6 + self.level
            self.tendencies[0] += 1
        elif action == "skill" and self.energy >= 2:
            self.energy -= 2
            damage = 10 + self.level * 2
            self.tendencies[1] += 1
        elif action == "item" and self.coins >= 5:
            self.coins -= 5
            self.stamina = min(100, self.stamina + 15)
            incoming = 0
            self.tendencies[3] += 1
        elif action == "defend":
            incoming = 3
            self.tendencies[2] += 1
        else:
            return

        self.boss_hp = max(0, self.boss_hp - damage)
        if self.boss_hp == 0:
            repeat_count = self.boss_wins[self.battle_region]
            divisor_shift = min(repeat_count, 15)
            experience_reward = max(
                1, (20 + self.battle_region * 10) >> divisor_shift
            )
            coin_reward = max(
                1, (20 + self.battle_region * 15) >> divisor_shift
            )
            self.boss_defeated_mask |= 1 << self.battle_region
            self.boss_wins[self.battle_region] += 1
            self.coins += coin_reward
            self.gain_experience(experience_reward)
            self.in_battle = False
            return

        self.stamina = max(0, self.stamina - incoming)
        if self.stamina == 0:
            self.in_battle = False
            self.stamina = 30

    @staticmethod
    def experience_for_level(level: int) -> int:
        if level <= 2:
            return 20
        if level <= 11:
            return 50
        return 100

    def gain_experience(self, amount: int) -> None:
        self.experience += amount
        xp = self.experience
        level = 1
        while level < 30:
            need = self.experience_for_level(level)
            if xp < need:
                break
            xp -= need
            level += 1
        self.level = level
        self.update_evolution()

    def update_evolution(self) -> None:
        if self.level >= 12 and self.form in (PetForm.ROOKIE_A, PetForm.ROOKIE_B):
            if self.form == PetForm.ROOKIE_A:
                self.form = (
                    PetForm.FINAL_A1
                    if self.tendencies[0] >= self.tendencies[1]
                    else PetForm.FINAL_A2
                )
            else:
                self.form = (
                    PetForm.FINAL_B1
                    if self.tendencies[2] >= self.tendencies[3]
                    else PetForm.FINAL_B2
                )
        elif self.level >= 3 and self.form == PetForm.EGG:
            agile = self.tendencies[0] + self.tendencies[1]
            steady = self.tendencies[2] + self.tendencies[3]
            self.form = PetForm.ROOKIE_A if agile >= steady else PetForm.ROOKIE_B

    def apply_task(self, duration_seconds: int, success: bool,
                   halved: bool = False) -> None:
        minutes = max(1, min(60, duration_seconds // 60))
        if success:
            exp_gain = minutes * 2
            coin_gain = minutes
            if halved:
                exp_gain = max(1, exp_gain // 2)
                coin_gain = max(1, coin_gain // 2)
            self.gain_experience(exp_gain)
            self.coins += coin_gain
            self.energy = min(self.max_energy(), self.energy + max(1, minutes // 2))
        else:
            self.gain_experience(max(1, (minutes + 1) // 2))

    def complete_ai_task(
        self, source: str, duration_seconds: int, halved: bool = False
    ) -> None:
        old_experience = self.experience
        old_coins = self.coins
        self.apply_task(duration_seconds, True, halved)
        self.ai_task_records[self.ai_task_record_index] = AiTaskRecord(
            source=source,
            duration_seconds=duration_seconds,
            experience_reward=self.experience - old_experience,
            coin_reward=self.coins - old_coins,
        )
        self.ai_task_record_index = (
            self.ai_task_record_index + 1
        ) % len(self.ai_task_records)
        self.ai_task_record_count = min(
            len(self.ai_task_records), self.ai_task_record_count + 1
        )

    def recent_ai_tasks(self) -> list[AiTaskRecord]:
        records = []
        for offset in range(1, self.ai_task_record_count + 1):
            index = (self.ai_task_record_index - offset) % len(
                self.ai_task_records
            )
            record = self.ai_task_records[index]
            if record is not None:
                records.append(record)
        return records
