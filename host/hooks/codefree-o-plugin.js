import { spawn, execFileSync } from "node:child_process"
import { homedir } from "node:os"
import { join } from "node:path"

const hookRoot = join(homedir(), ".ai-pet-hooks")
const stateScript = join(hookRoot, "hook_state.py")
const sendScript = join(hookRoot, "ai_pet_hook.py")

let pyCmd = null

function detectPython() {
  if (pyCmd) return pyCmd
  for (const [cmd, args] of [["py", ["-3", "--version"]], ["python", ["--version"]]]) {
    try {
      execFileSync(cmd, args, { stdio: "ignore", windowsHide: true })
      pyCmd = cmd
      return cmd
    } catch {}
  }
  return null
}

function stateAction(event) {
  if (event === "submitted") return ["begin"]
  if (event === "complete") return ["complete", "--result", "success"]
  return ["status", "--state", event]
}

function emit(event, session = "codefree-o-default") {
  const py = detectPython()
  if (!py) return

  const pyArgs = [
    ...(py === "py" ? ["-3"] : []),
    stateScript,
    ...stateAction(event),
    "--source", "codefree_o",
    "--session", session,
  ]

  const stateProc = spawn(py, pyArgs, {
    windowsHide: true,
    stdio: ["ignore", "pipe", "ignore"],
  })

  let buf = ""
  stateProc.stdout.on("data", (d) => { buf += d.toString() })
  stateProc.on("close", () => {
    const payload = buf.trim()
    if (!payload) return
    const sender = spawn(
      py,
      [...(py === "py" ? ["-3"] : []), sendScript, "send"],
      { windowsHide: true, stdio: ["pipe", "ignore", "ignore"] },
    )
    sender.stdin.end(payload)
    sender.unref()
  })
  stateProc.on("error", () => {})
  stateProc.unref()
}

function sessionId(event, fallback) {
  return (
    event?.properties?.sessionID ??
    event?.properties?.session_id ??
    event?.sessionID ??
    fallback
  )
}

export function createAiPetPlugin() {
  return async ({ project }) => {
    detectPython()
    const fallback = project?.id ?? "codefree-o-default"
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
}

export const AiPetPlugin = createAiPetPlugin()
