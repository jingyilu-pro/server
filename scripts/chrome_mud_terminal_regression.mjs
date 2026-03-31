import { spawn } from 'node:child_process'
import fs from 'node:fs/promises'
import os from 'node:os'
import path from 'node:path'

const port = Number(process.env.CDP_PORT ?? '9222')
const outputPrefix = process.env.MUD_REGRESSION_PREFIX ?? '.playwright-cli/page-2026-03-23T12-immersive-recheck'
const askTopic = process.env.MUD_ASK_TOPIC ?? '嘉元城'
const characterName = process.env.MUD_CHARACTER_NAME ?? '沉墨乙'
const chromePath = process.env.CHROME_PATH ?? 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe'
const targetUrl = process.env.MUD_TARGET_URL ?? 'http://127.0.0.1:5176/'

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms))

async function waitForJson(path, retries = 80) {
  for (let index = 0; index < retries; index += 1) {
    try {
      const response = await fetch(`http://127.0.0.1:${port}${path}`)
      if (response.ok) {
        return response.json()
      }
    } catch {}
    await sleep(250)
  }
  throw new Error(`timeout waiting for ${path}`)
}

function jsonString(value) {
  return JSON.stringify(value)
}

const userDataDir = await fs.mkdtemp(path.join(os.tmpdir(), 'mud-cdp-'))
const chrome = spawn(chromePath, [
  '--headless=new',
  '--disable-gpu',
  '--no-first-run',
  '--no-default-browser-check',
  `--remote-debugging-port=${port}`,
  '--window-size=430,932',
  `--user-data-dir=${userDataDir}`,
  targetUrl,
], {
  stdio: ['ignore', 'ignore', 'pipe'],
})

chrome.stderr.on('data', () => {})

let socket

try {
  const targets = await waitForJson('/json/list')
  const pageTarget = targets.find((item) => item.type === 'page')
  if (!pageTarget) {
    throw new Error('no page target')
  }

  socket = new WebSocket(pageTarget.webSocketDebuggerUrl)
  const pending = new Map()
  let nextId = 0
  const consoleErrors = []

  socket.onmessage = (event) => {
    const payload = JSON.parse(event.data)
    if (payload.id) {
      const request = pending.get(payload.id)
      if (!request) {
        return
      }
      pending.delete(payload.id)
      if (payload.error) {
        request.reject(new Error(payload.error.message))
      } else {
        request.resolve(payload.result)
      }
      return
    }

    if (payload.method === 'Runtime.consoleAPICalled') {
      const type = payload.params.type
      const text = (payload.params.args || []).map((arg) => arg.value ?? arg.description ?? '').join(' ')
      if (type === 'error') {
        consoleErrors.push(text)
      }
    }
    if (payload.method === 'Runtime.exceptionThrown') {
      consoleErrors.push(payload.params.exceptionDetails?.text || 'Runtime exception')
    }
    if (payload.method === 'Log.entryAdded' && payload.params.entry?.level === 'error') {
      consoleErrors.push(payload.params.entry.text || 'Log error')
    }
  }

  await new Promise((resolve, reject) => {
    socket.onopen = resolve
    socket.onerror = reject
  })

  function send(method, params = {}) {
    return new Promise((resolve, reject) => {
      const id = ++nextId
      pending.set(id, { resolve, reject })
      socket.send(JSON.stringify({ id, method, params }))
    })
  }

  async function evaluate(expression, awaitPromise = true) {
    const result = await send('Runtime.evaluate', {
      expression,
      awaitPromise,
      returnByValue: true,
    })
    return result.result?.value
  }

  async function waitFor(expression, label, retries = 80) {
    for (let index = 0; index < retries; index += 1) {
      const matched = await evaluate(expression)
      if (matched) {
        return matched
      }
      await sleep(250)
    }
    throw new Error(`timeout waiting for ${label}`)
  }

  async function captureScreenshot(filePath) {
    const screenshot = await send('Page.captureScreenshot', { format: 'png' })
    await fs.writeFile(filePath, Buffer.from(screenshot.data, 'base64'))
  }

  await send('Page.enable')
  await send('Runtime.enable')
  await send('Log.enable')
  await send('Page.bringToFront')
  await waitFor("document.readyState === 'complete'", 'document ready')
  await waitFor("Boolean(document.querySelector('input[autocomplete=\"username\"]'))", 'login form')

  const account = `mud${Date.now().toString().slice(-8)}`
  const password = 'abc123'

  await evaluate(`(() => {
  const set = (selector, value) => {
    const element = document.querySelector(selector)
    if (!element) return false
    element.value = value
    element.dispatchEvent(new Event('input', { bubbles: true }))
    element.dispatchEvent(new Event('change', { bubbles: true }))
    return true
  }
  set('input[autocomplete="username"]', ${jsonString(account)})
  set('input[type="password"]', ${jsonString(password)})
  const button = [...document.querySelectorAll('button')].find((item) => item.textContent.includes('新开一卷'))
  button?.click()
  return true
})()`)

  await waitFor("document.body.innerText.includes('立下名帖') || document.body.innerText.includes('七玄门外场')", 'create or game view')

  const createNeeded = await evaluate("document.body.innerText.includes('立下名帖')")
  if (createNeeded) {
    await evaluate(`(() => {
    const set = (element, value) => {
      element.value = value
      element.dispatchEvent(new Event('input', { bubbles: true }))
      element.dispatchEvent(new Event('change', { bubbles: true }))
    }
    const nameInput = [...document.querySelectorAll('input')].find((item) => item.placeholder?.includes('韩立'))
    if (nameInput) set(nameInput, ${jsonString(characterName)})
    document.querySelector('.origin-card')?.click()
    const cards = [...document.querySelectorAll('.origin-card')]
    if (cards[5]) cards[5].click()
    const createButton = [...document.querySelectorAll('button')].find((item) => item.textContent.includes('落笔成名'))
    createButton?.click()
    return true
  })()`)
  }

  await waitFor("Boolean(document.querySelector('.mobile-layout--terminal'))", 'game view')
  await sleep(1200)

  const beforeImage = `${outputPrefix}-before.png`
  const afterImage = `${outputPrefix}-after.png`
  const resultJson = `${outputPrefix}.json`

  await captureScreenshot(beforeImage)

  const before = await evaluate(`(() => ({
  shellClass: document.querySelector('.shell')?.className || '',
  title: document.querySelector('.scene-board-title')?.textContent?.trim() || '',
  kicker: document.querySelector('.scene-board-kicker')?.textContent?.trim() || '',
  mission: document.querySelector('.scene-board-subtitle')?.textContent?.trim() || '',
  sceneLinkCount: document.querySelectorAll('.scene-link-strip').length,
  hasSettingsButton: [...document.querySelectorAll('.mode-button')].some((item) => item.textContent.includes('卷末')),
  hasSceneActionPanel: [...document.querySelectorAll('.story-panel-title')].some((item) => item.textContent.includes('可做')),
  orientationLabels: [...document.querySelectorAll('.scene-orientation-card .orientation-label')].map((item) => item.textContent.trim()),
  storyTail: [...document.querySelectorAll('.story-log .story-line .story-text, .story-log .story-panel .story-panel-summary')].slice(-8).map((item) => item.textContent.trim()),
  dock: [...document.querySelectorAll('.dock-nav-button span')].map((item) => item.textContent.trim()),
}))()`)

  await evaluate(`(() => {
  const commandButton = [...document.querySelectorAll('.mode-button')].find((item) =>
    item.textContent.includes('落令') || item.textContent.includes('指令'),
  )
  commandButton?.click()
  const input = document.querySelector('.command-form input')
  input.value = ${jsonString(`ask 厉飞雨 ${askTopic}`)}
  input.dispatchEvent(new Event('input', { bubbles: true }))
  input.dispatchEvent(new Event('change', { bubbles: true }))
  document.querySelector('.command-form button')?.click()
  return true
})()`)

  await sleep(1200)

  const askResult = await evaluate(`(() => ({
  shellClass: document.querySelector('.shell')?.className || '',
  hasSceneActionPanel: [...document.querySelectorAll('.story-panel-title')].some((item) => item.textContent.includes('可做')),
  storyTail: [...document.querySelectorAll('.story-log .story-line .story-text, .story-log .story-panel .story-panel-line, .story-log .story-panel .story-panel-summary')].slice(-12).map((item) => item.textContent.trim()),
}))()`)

  await evaluate(`(() => {
  const input = document.querySelector('.command-form input')
  input.value = 'go south'
  input.dispatchEvent(new Event('input', { bubbles: true }))
  input.dispatchEvent(new Event('change', { bubbles: true }))
  document.querySelector('.command-form button')?.click()
  return true
})()`)

  await sleep(1200)
  await captureScreenshot(afterImage)

  const after = await evaluate(`(() => ({
  shellClass: document.querySelector('.shell')?.className || '',
  title: document.querySelector('.scene-board-title')?.textContent?.trim() || '',
  kicker: document.querySelector('.scene-board-kicker')?.textContent?.trim() || '',
  sceneLinkCount: document.querySelectorAll('.scene-link-strip').length,
  hasSettingsButton: [...document.querySelectorAll('.mode-button')].some((item) => item.textContent.includes('卷末')),
  hasSceneActionPanel: [...document.querySelectorAll('.story-panel-title')].some((item) => item.textContent.includes('可做')),
  orientationLabels: [...document.querySelectorAll('.scene-orientation-card .orientation-label')].map((item) => item.textContent.trim()),
  storyTail: [...document.querySelectorAll('.story-log .story-line .story-text, .story-log .story-panel .story-panel-line, .story-log .story-panel .story-panel-summary')].slice(-12).map((item) => item.textContent.trim()),
}))()`)

  const summary = {
    account,
    before,
    askResult,
    after,
    consoleErrors,
    screenshots: [beforeImage, afterImage],
  }

  await fs.writeFile(resultJson, JSON.stringify(summary, null, 2))
  console.log(JSON.stringify(summary, null, 2))
  socket.close()
} finally {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.close()
  }
  if (!chrome.killed) {
    chrome.kill('SIGKILL')
  }
  await sleep(500)
  try {
    await fs.rm(userDataDir, { recursive: true, force: true })
  } catch {}
}
