from dataclasses import dataclass


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

    def interact(self) -> None:
        self.mood = min(100, self.mood + 5)

    def feed(self) -> bool:
        if self.coins < 10 or self.stamina >= 100:
            return False
        self.coins -= 10
        self.stamina = min(100, self.stamina + 20)
        return True

    def start_exploration(self, region: int) -> bool:
        if region not in (0, 1, 2) or self.energy < 3:
            return False
        self.energy -= 3
        self.active_region = region
        return True

    def apply_task(self, duration_seconds: int, success: bool) -> None:
        minutes = max(1, min(60, duration_seconds // 60))
        if success:
            self.experience += minutes * 2
            self.coins += minutes
            self.energy = min(999, self.energy + max(1, minutes // 2))
        else:
            self.experience += max(1, (minutes + 1) // 2)
