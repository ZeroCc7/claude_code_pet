#include "pet_renderer.h"

#include "assets/pet_effects.h"
#include "assets/pet_sprites.h"

void PetRenderer::draw(Adafruit_GFX& tft, PetForm form, int16_t x, int16_t y,
                       uint32_t now, PetEffect effect,
                       uint32_t effectElapsed, bool evolution) {
  const PetSpriteFrame* frames = nullptr;
  uint8_t width = kPetSpriteWidth;
  uint8_t height = kPetSpriteHeight;
  if (form == PetForm::Egg) {
    frames = kPet_egg_frames;
  } else if (form == PetForm::RookieA) {
    frames = kPet_rookie_a_frames;
  } else if (form == PetForm::RookieB) {
    frames = kPet_rookie_b_frames;
  } else if (form == PetForm::FinalA1) {
    frames = kPet_final_a1_frames;
    width = kFinalPetSpriteWidth;
    height = kFinalPetSpriteHeight;
  } else if (form == PetForm::FinalA2) {
    frames = kPet_final_a2_frames;
    width = kFinalPetSpriteWidth;
    height = kFinalPetSpriteHeight;
  } else if (form == PetForm::FinalB1) {
    frames = kPet_final_b1_frames;
    width = kFinalPetSpriteWidth;
    height = kFinalPetSpriteHeight;
  } else if (form == PetForm::FinalB2) {
    frames = kPet_final_b2_frames;
    width = kFinalPetSpriteWidth;
    height = kFinalPetSpriteHeight;
  }
  if (evolution && form >= PetForm::FinalA1) {
    if (form == PetForm::FinalA1) frames = kPet_final_a1_evolution_frames;
    else if (form == PetForm::FinalA2) frames = kPet_final_a2_evolution_frames;
    else if (form == PetForm::FinalB1) frames = kPet_final_b1_evolution_frames;
    else if (form == PetForm::FinalB2) frames = kPet_final_b2_evolution_frames;
  }
  if (!frames) {
    return;
  }

  if (effect != PetEffect::None && form >= PetForm::FinalA1) {
    drawEffect(tft, form, x, y, effectElapsed);
  }
  const PetSpriteFrame& frame =
      frames[(now / 400) % kPetSpriteFrameCount];
  tft.drawRGBBitmap(x, y, frame.pixels, frame.mask, width, height);
}

void PetRenderer::drawEffect(Adafruit_GFX& tft, PetForm form, int16_t x,
                             int16_t y, uint32_t effectElapsed) {
  const PetEffectFrame* frames = nullptr;
  switch (form) {
    case PetForm::FinalA1:
      frames = kPetEffect_final_a1_frames;
      break;
    case PetForm::FinalA2:
      frames = kPetEffect_final_a2_frames;
      break;
    case PetForm::FinalB1:
      frames = kPetEffect_final_b1_frames;
      break;
    case PetForm::FinalB2:
      frames = kPetEffect_final_b2_frames;
      break;
    default:
      return;
  }
  const PetEffectFrame& frame =
      frames[(effectElapsed / 200) % kPetEffectFrameCount];
  tft.drawRGBBitmap(x, y, frame.pixels, frame.mask, kPetEffectWidth,
                    kPetEffectHeight);
}
