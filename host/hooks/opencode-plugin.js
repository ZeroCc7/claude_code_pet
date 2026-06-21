import { spawn } from "node:child_process"
import { homedir } from "node:os"
import { join } from "node:path"

const script = join(homedir(), ".ai-pet-hooks", "opencode-hook.ps1")

function emit(event, session = "opencode-default") {
  const child = spawn(
    "powershell",
    [
      "-NoProfile",
      "-ExecutionPolicy",
      "Bypass",
      "-File",
      script,
      "-Event",
      event,
      "-Session",
      session,
    ],
    { windowsHide: true, stdio: "ignore" },
  )
  child.unref()
}

function sessionId(event, fallback) {
  return (
    event?.properties?.sessionID ??
    event?.properties?.session_id ??
    event?.sessionID ??
    fallback
  )
}

export const AiPetPlugin = async ({ project }) => {
  const fallback = project?.id ?? "opencode-default"
  return {
    event: async ({ event }) => {
      const id = sessionId(event, fallback)
      switch (event.type) {
        case "session.created":
          emit("submitted", id)
          break
        case "session.status":
          emit("thinking", id)
          break
        case "permission.asked":
          emit("waiting", id)
          break
        case "file.edited":
          emit("editing", id)
          break
        case "session.error":
          emit("blocked", id)
          break
        case "session.idle":
          emit("complete", id)
          break
        case "session.deleted":
          emit("idle", id)
          break
      }
    },
    "tool.execute.before": async (input) => {
      emit(
        /write|edit|patch/i.test(input.tool) ? "editing" : "tool",
        input.sessionID ?? fallback,
      )
    },
  }
}
