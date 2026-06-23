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

    def use_item(self, item: ItemType) -> bool:
        if self.items[item] == 0:
            return False
        if item == ItemType.SPIRIT_HERB:
            if self.energy >= 20:
                return False
            self.energy = min(20, self.energy + 3)
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
        if self.energy >= 20:
            self.energy_recovery_seconds = 0
            return changed

        self.energy_recovery_seconds += seconds
        while self.energy_recovery_seconds >= 300 and self.energy < 20:
            self.energy_recovery_seconds -= 300
            self.energy += 1
            changed = True
        if self.energy >= 20:
            self.energy_recovery_seconds = 0
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

    def gain_experience(self, amount: int) -> None:
        self.experience += amount
        self.level = min(30, self.experience // 20 + 1)
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
            self.energy = min(20, self.energy + max(1, minutes // 2))
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
