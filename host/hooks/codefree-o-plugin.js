import { spawn } from "node:child_process"
import { homedir } from "node:os"
import { join } from "node:path"

const script = join(homedir(), ".ai-pet-hooks", "codefree-o-hook.ps1")

function emit(event) {
  const child = spawn(
    "powershell",
    ["-NoProfile", "-ExecutionPolicy", "Bypass", "-File", script, "-Event", event],
    { windowsHide: true, stdio: "ignore" },
  )
  child.unref()
}

export function createAiPetPlugin() {
  return async () => ({
    event: async ({ event }) => {
      if (event.type === "session.created") emit("start")
      if (event.type === "session.idle") emit("end")
    },
  })
}

export const AiPetPlugin = createAiPetPlugin()
