from __future__ import annotations

import random
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


TECHNIQUE_MAX_LEVEL = 9
TECHNIQUE_THRESHOLDS = (0, 10, 22, 38, 58, 82, 112, 148, 190)
TECHNIQUE_COSTS = (50, 90, 150, 240, 360, 520, 720, 960, 1250)
TECHNIQUE_SWORD = 0
TECHNIQUE_DAN = 1
TECHNIQUE_BODY = 2
TECHNIQUE_SPIRIT = 3


@dataclass(frozen=True)
class AiTaskRecord:
    source: str
    duration_seconds: int
    experience_reward: int
    coin_reward: int


@dataclass(frozen=True)
class RegionConfig:
    name: str
    boss_name: str
    base_boss_hp: int
    hp_scale_first10: int
    hp_scale_later: int
    base_boss_damage: int
    damage_scale_first10: int
    damage_scale_later: int
    base_xp: int
    base_coins: int
    tendency_base: int
    unlock_level: int
    unlock_coins: int
    prerequisite: int
    reward_bias: tuple[int, int, int, int]


REGIONS = (
    RegionConfig(
        "青云山道", "青云妖狼", 60, 25, 6, 13, 15, 4,
        6, 15, 2, 0, 0, 0, (0, 0, 0, 0),
    ),
    RegionConfig(
        "青竹灵境", "竹灵守卫", 90, 27, 7, 17, 17, 5,
        8, 18, 3, 5, 100, 0, (0, 2, 0, 2),
    ),
    RegionConfig(
        "云海剑台", "云海剑灵", 125, 30, 8, 22, 19, 6,
        10, 22, 3, 12, 300, 1, (3, 0, 0, 0),
    ),
    RegionConfig(
        "丹霞药谷", "丹谷兽王", 165, 33, 9, 28, 22, 7,
        12, 28, 4, 18, 600, 2, (0, 3, 0, 1),
    ),
    RegionConfig(
        "玄河古战场", "古战场霸主", 220, 36, 10, 36, 25, 8,
        15, 35, 5, 25, 1000, 3, (2, 0, 2, 0),
    ),
)


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
    region_progress: list[int] = field(default_factory=lambda: [0, 0, 0, 0, 0])
    regions_unlocked: int = 0b00001
    boss_defeated_mask: int = 0
    boss_wins: list[int] = field(default_factory=lambda: [0, 0, 0, 0, 0])
    tendencies: list[int] = field(default_factory=lambda: [0, 0, 0, 0])
    technique_levels: list[int] = field(default_factory=lambda: [0, 0, 0, 0])
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
    qingyun_event_order: int = 0xE4
    qingyun_boss_unlocked: bool = False
    qingyun_boss_defeated: bool = False
    qingyun_boss_wins: int = 0
    qingyun_round: int = 1
    qingyun_misses: int = 0
    has_qingyun_sword: bool = False
    region_treasure: list[int] = field(default_factory=lambda: [0, 0, 0, 0, 0])
    region_misses: list[int] = field(default_factory=lambda: [0, 0, 0, 0, 0])
    stamina_recovery_seconds: int = 0
    last_qingyun_experience: int = 0
    last_qingyun_coins: int = 0
    last_qingyun_items: list[int] = field(default_factory=lambda: [0] * 4)
    last_qingyun_sword: bool = False
    last_boss_treasure: bool = False
    battle_round: int = 0
    battle_attack_talisman: bool = False
    battle_guard_talisman: bool = False
    battle_shield: int = 0
    last_battle_result: BattleResult = BattleResult.INACTIVE

    def __post_init__(self) -> None:
        self.region_treasure = (self.region_treasure + [0] * 5)[:5]
        self.region_misses = (self.region_misses + [0] * 5)[:5]
        if self.has_qingyun_sword:
            self.region_treasure[0] = 1
        else:
            self.has_qingyun_sword = bool(self.region_treasure[0])
        if self.qingyun_misses:
            self.region_misses[0] = self.qingyun_misses
        else:
            self.qingyun_misses = self.region_misses[0]

    def technique_level(self, index: int) -> int:
        return self.technique_levels[index]

    def upgrade_technique(self, index: int) -> bool:
        level = self.technique_levels[index]
        if level >= TECHNIQUE_MAX_LEVEL:
            return False
        target = level + 1
        threshold = TECHNIQUE_THRESHOLDS[target - 1]
        cost = TECHNIQUE_COSTS[target - 1]
        if self.tendencies[index] < threshold or self.coins < cost:
            return False
        self.coins -= cost
        self.technique_levels[index] = target
        return True

    def max_energy(self) -> int:
        if self.form >= PetForm.FINAL_A1:
            base = 80
        elif self.form >= PetForm.ROOKIE_A:
            base = 40
        else:
            base = 20
        if self.technique_levels[TECHNIQUE_SPIRIT] >= 9:
            base += max(10, base * 30 // 100)
        return base

    def recovery_interval_seconds(self) -> int:
        return 180 if self.technique_levels[TECHNIQUE_DAN] >= 9 else 300

    def spirit_herb_restore_amount(self) -> int:
        level = self.technique_levels[TECHNIQUE_DAN]
        return 3 + (1 if level >= 1 else 0) + (1 if level >= 6 else 0)

    def recovery_pill_restore_amount(self) -> int:
        level = self.technique_levels[TECHNIQUE_DAN]
        return 20 + (5 if level >= 2 else 0) + (5 if level >= 7 else 0)

    def use_item(self, item: ItemType) -> bool:
        if self.items[item] == 0:
            return False
        if item == ItemType.SPIRIT_HERB:
            if self.energy >= self.max_energy():
                return False
            self.energy = min(
                self.max_energy(),
                self.energy + self.spirit_herb_restore_amount(),
            )
        elif item == ItemType.RECOVERY_PILL:
            if self.stamina >= 100:
                return False
            self.stamina = min(100, self.stamina + self.recovery_pill_restore_amount())
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
            interval = self.recovery_interval_seconds()
            while (
                self.energy_recovery_seconds >= interval
                and self.energy < self.max_energy()
            ):
                self.energy_recovery_seconds -= interval
                self.energy += 1
                changed = True
            if self.energy >= self.max_energy():
                self.energy_recovery_seconds = 0

        if self.stamina >= 100:
            self.stamina_recovery_seconds = 0
        else:
            self.stamina_recovery_seconds += seconds
            while (
                self.stamina_recovery_seconds >= self.recovery_interval_seconds()
                and self.stamina < 100
            ):
                self.stamina_recovery_seconds -= self.recovery_interval_seconds()
                self.stamina = min(100, self.stamina + 5)
                changed = True
            if self.stamina >= 100:
                self.stamina_recovery_seconds = 0
        return changed

    def region_unlocked(self, region: int) -> bool:
        if not 0 <= region < len(REGIONS):
            return False
        return bool(self.regions_unlocked & (1 << region)) or (
            region == 0 or bool(self.boss_defeated_mask & (1 << (region - 1)))
        )

    def try_unlock_region(self, region: int) -> bool:
        if not 0 <= region < len(REGIONS):
            return False
        if self.region_unlocked(region):
            return True
        cfg = REGIONS[region]
        if (
            self.level < cfg.unlock_level
            or self.coins < cfg.unlock_coins
            or self.boss_wins[cfg.prerequisite] == 0
        ):
            return False
        self.coins -= cfg.unlock_coins
        self.regions_unlocked |= 1 << region
        return True

    def select_region(self, region: int) -> None:
        if self.region_unlocked(region):
            self.active_region = region

    def start_exploration(self, region: int) -> bool:
        if (
            not 0 <= region < len(REGIONS)
            or not self.region_unlocked(region)
            or self.energy < 3
        ):
            return False
        self.energy -= 3
        self.active_region = region
        return True

    def boss_energy_requirement(self) -> int:
        return 4 if self.technique_levels[TECHNIQUE_DAN] >= 4 else 5

    def can_use_region_token_for_boss(self) -> bool:
        return (
            self.items[ItemType.QINGYUN_TOKEN] > 0
            and self.energy >= self.boss_energy_requirement()
        )

    def use_region_token_for_boss(self) -> bool:
        if not self.can_use_region_token_for_boss():
            return False
        self.items[ItemType.QINGYUN_TOKEN] -= 1
        self.qingyun_progress = 100
        self.qingyun_boss_unlocked = True
        self.adventure_phase = AdventurePhase.BOSS_READY
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
        # Shuffle event order (Fisher-Yates)
        perm = [0, 1, 2, 3]
        for i in range(3, 0, -1):
            j = random.randint(0, i)
            perm[i], perm[j] = perm[j], perm[i]
        self.qingyun_event_order = (
            perm[0] | (perm[1] << 2) | (perm[2] << 4) | (perm[3] << 6)
        )
        return True

    def stop_qingyun_adventure(self) -> None:
        self.adventure_phase = (
            AdventurePhase.BOSS_READY
            if self.qingyun_progress >= 100
            else AdventurePhase.IDLE
        )
        self.current_event = QingyunEvent.NONE

    def tick_qingyun_adventure(self, seed: int) -> AdventureTick:
        if self.adventure_phase == AdventurePhase.RESULT:
            return AdventureTick.INACTIVE
        if self.adventure_phase != AdventurePhase.ADVANCING:
            return AdventureTick.INACTIVE
        skip_energy_cost = (
            self.technique_levels[TECHNIQUE_SPIRIT] >= 6 and seed % 100 < 10
        )
        if not skip_energy_cost:
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
                event_idx = (self.qingyun_event_order >> (index * 2)) & 0x03
                self.current_event = events[event_idx]
                self._auto_resolve_event(seed)
                self.adventure_phase = AdventurePhase.RESULT
                return AdventureTick.EVENT_TRIGGERED
        if self.qingyun_progress >= 100:
            self.qingyun_boss_unlocked = True
            self.adventure_phase = AdventurePhase.BOSS_READY
            return AdventureTick.BOSS_UNLOCKED
        if self.energy == 0:
            self.stop_qingyun_adventure()
            return AdventureTick.ENERGY_DEPLETED
        return AdventureTick.ADVANCED

    def _auto_resolve_event(self, seed: int) -> None:
        result = EventResult.CONTINUED
        if self.current_event == QingyunEvent.SPIRIT_HERB:
            self._add_item(ItemType.SPIRIT_HERB)
            self._add_tendency(3, 1)
            result = EventResult.ITEM_GAINED
        elif self.current_event == QingyunEvent.DEMON_BEAST:
            score = (
                self.level
                + self.stamina // 10
                + self.tendencies[0] // 4
                + seed % 10
            )
            if score >= 15:
                self._add_tendency(0, 2)
                self.gain_experience(6)
                self.coins += 5
                result = EventResult.REWARD_GAINED
            else:
                self.stamina = max(
                    0, self.stamina - self._qingyun_event_damage(12)
                )
                result = EventResult.STAMINA_LOST
        elif self.current_event == QingyunEvent.WOUNDED_CULTIVATOR:
            score = (
                self.level
                + self.stamina // 10
                + self.tendencies[3] // 2
                + seed % 10
            )
            if score >= 22:
                self._add_item(ItemType.QINGYUN_TOKEN)
            else:
                self._add_item(ItemType.RECOVERY_PILL)
            self._add_tendency(3, 2)
            result = EventResult.ITEM_GAINED
        elif self.current_event == QingyunEvent.SHORTCUT:
            score = (
                self.level
                + self.energy
                + self.tendencies[1] // 3
                + seed % 10
            )
            if score >= 17:
                self._add_tendency(1, 2)
                self.qingyun_progress = min(
                    100, self.qingyun_progress + 8 + seed % 8
                )
                result = EventResult.PROGRESS_GAINED
            else:
                self.stamina = max(
                    0, self.stamina - self._qingyun_event_damage(10)
                )
                result = EventResult.STAMINA_LOST
        self.current_event_result = result

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

    def _add_item(self, item: ItemType) -> None:
        self.items[item] = min(65535, self.items[item] + 1)

    def _add_tendency(self, slot: int, amount: int) -> None:
        if 0 <= slot < 4:
            self.tendencies[slot] = min(100, self.tendencies[slot] + amount)

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
        body_level = self.technique_levels[TECHNIQUE_BODY]
        self.battle_shield = (
            (3 if body_level >= 2 else 0)
            + (4 if body_level >= 5 else 0)
            + (5 if body_level >= 8 else 0)
        )
        return True

    def attack_damage(self, seed: int) -> int:
        return self.qingyun_attack_damage(seed)

    def qingyun_attack_damage(self, seed: int) -> int:
        damage = (
            6
            + self.level
            + min(10, self.tendencies[0] * 3 // 8)
            + min(8, self.tendencies[1] // 3)
        )
        if self.form in (PetForm.ROOKIE_A, PetForm.FINAL_A1):
            damage += 1
        elif self.form == PetForm.FINAL_A2:
            damage += 2
        sword_level = self.technique_levels[TECHNIQUE_SWORD]
        sword_bonus = (
            (3 if sword_level >= 1 else 0)
            + (3 if sword_level >= 3 else 0)
            + (4 if sword_level >= 5 else 0)
            + (5 if sword_level >= 7 else 0)
        )
        damage = damage * (100 + sword_bonus) // 100
        critical_rate = (
            5
            + min(20, self.tendencies[0] // 3)
            + (2 if sword_level >= 2 else 0)
            + (3 if sword_level >= 6 else 0)
        )
        if self._has_region_treasure(2):
            critical_rate = min(100, critical_rate + 15)
        if seed % 100 < critical_rate:
            damage = damage * (220 if sword_level >= 9 else 200) // 100
        if self.battle_attack_talisman:
            damage = damage * 120 // 100
        if self._has_region_treasure(0):
            damage = damage * 110 // 100
        pierce = (1 if sword_level >= 4 else 0) + (1 if sword_level >= 8 else 0)
        damage += pierce * 2
        return max(1, damage)

    def incoming_damage(self, seed: int) -> int:
        return self.qingyun_incoming_damage(seed)

    def qingyun_incoming_damage(self, seed: int) -> int:
        body_level = self.technique_levels[TECHNIQUE_BODY]
        spirit_level = self.technique_levels[TECHNIQUE_SPIRIT]
        dodge_rate = (
            5
            + min(20, self.tendencies[2] // 3)
            + (3 if body_level >= 6 else 0)
            + (2 if spirit_level >= 2 else 0)
            + (3 if spirit_level >= 7 else 0)
        )
        if self._has_region_treasure(1):
            dodge_rate = min(100, dodge_rate + 10)
        if (seed // 100) % 100 < dodge_rate:
            return 0
        damage = max(
            1,
            self._region_config().base_boss_damage
            - min(5, self.tendencies[2] // 6),
        )
        if self.form in (PetForm.ROOKIE_B, PetForm.FINAL_B1):
            damage = max(1, damage - 1)
        elif self.form == PetForm.FINAL_B2:
            damage = max(1, damage - 2)
        if self.battle_guard_talisman:
            damage = max(1, damage * 80 // 100)
        damage = max(1, damage * self._qingyun_damage_percent() // 100)
        if self._has_region_treasure(0):
            damage = max(1, damage * 90 // 100)
        body_reduction = (
            (3 if body_level >= 1 else 0)
            + (4 if body_level >= 4 else 0)
            + (5 if body_level >= 7 else 0)
        )
        if self.stamina < 20 and body_level >= 9:
            body_reduction += 10
        if body_reduction:
            damage = max(1, damage * (100 - body_reduction) // 100)
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
            if self.technique_levels[TECHNIQUE_SPIRIT] >= 3:
                self.energy = min(self.max_energy(), self.energy + 1)
            cfg = self._region_config()
            boss_bonus = cfg.tendency_base + min(3, self.qingyun_round // 5)
            for i in range(4):
                self._add_tendency(i, boss_bonus + cfg.reward_bias[i])
            self._reset_qingyun_run()
            self._finish_qingyun_battle(BattleResult.VICTORY, reset_hp=False)
            return BattleResult.VICTORY

        incoming = self.qingyun_incoming_damage(seed)
        if self.battle_shield:
            blocked = min(self.battle_shield, incoming)
            self.battle_shield -= blocked
            incoming -= blocked
        self.stamina = max(0, self.stamina - incoming)
        spirit_level = self.technique_levels[TECHNIQUE_SPIRIT]
        affinity_rate = (
            min(20, self.tendencies[3] // 3)
            + (3 if spirit_level >= 1 else 0)
            + (4 if spirit_level >= 5 else 0)
            + (3 if spirit_level >= 8 else 0)
        )
        if (
            self.stamina > 0
            and (seed // 10000) % 100 < affinity_rate
        ):
            heal = 2 + (1 if spirit_level >= 4 else 0)
            self.stamina = min(100, self.stamina + heal)
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
        return self._region_config().base_boss_hp * self._qingyun_health_percent() // 100

    def qingyun_completion_experience(self) -> int:
        cfg = self._region_config()
        round_number = min(50, max(1, self.qingyun_round))
        return (
            cfg.base_xp
            + min(round_number, 10)
            + max(0, round_number - 10) // 5
        )

    def qingyun_completion_coins(self) -> int:
        cfg = self._region_config()
        round_number = min(50, max(1, self.qingyun_round))
        return (
            cfg.base_coins
            + 2 * min(round_number, 10)
            + max(0, round_number - 10)
        )

    def _region_config(self) -> RegionConfig:
        region = self.active_region if self.active_region is not None else 0
        if not 0 <= region < len(REGIONS):
            region = 0
        return REGIONS[region]

    def _qingyun_health_percent(self) -> int:
        cfg = self._region_config()
        round_number = min(50, max(1, self.qingyun_round))
        return (
            100
            + min(round_number - 1, 9) * cfg.hp_scale_first10
            + max(0, round_number - 10) * cfg.hp_scale_later
        )

    def _qingyun_damage_percent(self) -> int:
        cfg = self._region_config()
        round_number = min(50, max(1, self.qingyun_round))
        return (
            100
            + min(round_number - 1, 9) * cfg.damage_scale_first10
            + max(0, round_number - 10) * cfg.damage_scale_later
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
        self._roll_region_treasure(seed)

    def _roll_region_treasure(self, seed: int) -> None:
        region = self.active_region if self.active_region is not None else 0
        if not 0 <= region < len(REGIONS):
            region = 0
        self.last_boss_treasure = False
        self.last_qingyun_sword = False
        if self.region_treasure[region]:
            self.region_misses[region] = 0
            self._sync_qingyun_treasure_aliases()
            return
        chance = min(10, max(1, self.qingyun_round))
        if self.region_misses[region] >= 19 or seed % 100 < chance:
            self.region_treasure[region] = 1
            self.last_boss_treasure = True
            self.region_misses[region] = 0
            self._apply_region_treasure_bonus(region)
        else:
            self.region_misses[region] = min(255, self.region_misses[region] + 1)
        self._sync_qingyun_treasure_aliases()

    def _has_region_treasure(self, region: int) -> bool:
        if region == 0 and self.has_qingyun_sword:
            return True
        return 0 <= region < len(self.region_treasure) and bool(
            self.region_treasure[region]
        )

    def _sync_qingyun_treasure_aliases(self) -> None:
        self.has_qingyun_sword = bool(self.region_treasure[0])
        self.qingyun_misses = self.region_misses[0]
        self.last_qingyun_sword = self.last_boss_treasure and self.active_region in (
            None,
            0,
        )

    def _apply_region_treasure_bonus(self, region: int) -> None:
        if region == 3:
            self.stamina = min(100, self.stamina + 20)
        elif region == 4:
            for i in range(4):
                self._add_tendency(i, 10)

    def _reset_qingyun_run(self) -> None:
        self.qingyun_progress = 0
        self.qingyun_event_mask = 0
        self.qingyun_event_order = 0
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
        self.battle_shield = 0

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

    def apply_task(self, source: str, duration_seconds: int, success: bool,
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
            tendency_gain = max(1, min(4, minutes // 5))
            slot = {"codex": 0, "claude-code": 1, "opencode": 2}.get(source, 3)
            self._add_tendency(slot, tendency_gain)
        else:
            self.gain_experience(max(1, (minutes + 1) // 2))

    def complete_ai_task(
        self, source: str, duration_seconds: int, halved: bool = False
    ) -> None:
        old_experience = self.experience
        old_coins = self.coins
        self.apply_task(source, duration_seconds, True, halved)
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
