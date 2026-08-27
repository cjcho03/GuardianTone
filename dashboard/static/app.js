const $ = (id) => document.getElementById(id)

function fmt(value, digits = 1) {
  return value === null || value === undefined
    ? "--"
    : Number(value).toFixed(digits)
}

function formatTime(value) {
  if (!value) return "--"

  return new Date(value).toLocaleString([], {
    hour: "numeric",
    minute: "2-digit",
    second: "2-digit"
  })
}

function drawLineChart(canvas, rows, series) {
  const ctx = canvas.getContext("2d")
  const rect = canvas.getBoundingClientRect()
  const dpr = window.devicePixelRatio || 1

  canvas.width = Math.max(300, rect.width * dpr)
  canvas.height = Math.max(180, rect.height * dpr)
  ctx.scale(dpr, dpr)

  const width = rect.width
  const height = rect.height
  ctx.clearRect(0, 0, width, height)

  const pad = {
    l: 42,
    r: 16,
    t: 16,
    b: 28
  }

  const plotW = width - pad.l - pad.r
  const plotH = height - pad.t - pad.b

  const values = []

  for (const row of rows) {
    for (const s of series) {
      const value = Number(row[s.key])

      if (Number.isFinite(value)) {
        values.push(value)
      }
    }
  }

  if (!values.length) {
    ctx.fillStyle = "#666"
    ctx.font = "13px Arial"
    ctx.fillText(
      "Waiting for telemetry...",
      pad.l,
      pad.t + 25
    )

    return
  }

  let min = Math.min(...values)
  let max = Math.max(...values)

  if (min === max) {
    min -= 1
    max += 1
  }

  const margin = (max - min) * 0.12

  min -= margin
  max += margin

  ctx.strokeStyle = "#ddd"
  ctx.lineWidth = 1
  ctx.fillStyle = "#666"
  ctx.font = "11px Arial"

  for (let i = 0; i <= 4; i++) {
    const y = pad.t + (plotH * i / 4)

    ctx.beginPath()
    ctx.moveTo(pad.l, y)
    ctx.lineTo(width - pad.r, y)
    ctx.stroke()

    const value =
      max - (max - min) * i / 4

    ctx.fillText(
      value.toFixed(1),
      3,
      y + 4
    )
  }

  const palette = [
    "#2f6fed",
    "#2f8f5b",
    "#c97818"
  ]

  series.forEach((s, si) => {
    ctx.strokeStyle =
      palette[si % palette.length]

    ctx.lineWidth = 2
    ctx.beginPath()

    let started = false

    rows.forEach((row, i) => {
      const value = Number(row[s.key])

      if (!Number.isFinite(value)) {
        return
      }

      const x =
        pad.l +
        (
          rows.length <= 1
            ? 0
            : (i / (rows.length - 1)) * plotW
        )

      const y =
        pad.t +
        ((max - value) / (max - min)) *
          plotH

      if (!started) {
        ctx.moveTo(x, y)
        started = true
      } else {
        ctx.lineTo(x, y)
      }
    })

    ctx.stroke()
  })

  series.forEach((s, i) => {
    const x = pad.l + i * 118

    ctx.fillStyle =
      palette[i % palette.length]

    ctx.fillRect(
      x,
      height - 15,
      9,
      3
    )

    ctx.fillStyle = "#555"

    ctx.fillText(
      s.label,
      x + 14,
      height - 11
    )
  })
}

async function loadDashboard() {
  try {
    const [
      latestRes,
      historyRes,
      alertsRes,
      statusRes
    ] = await Promise.all([
      fetch("/api/latest"),
      fetch("/api/history?limit=90"),
      fetch("/api/alerts?limit=8"),
      fetch("/api/status")
    ])

    const latest =
      await latestRes.json()

    const history =
      await historyRes.json()

    const alerts =
      await alertsRes.json()

    const status =
      await statusRes.json()

    const connection =
      $("connectionPill")

    if (status.consumerError) {
      connection.textContent =
        "Azure consumer error"

      connection.className =
        "connection-status error"
    } else if (latest) {
      connection.textContent =
        "Live telemetry connected"

      connection.className =
        "connection-status live"
    } else {
      connection.textContent =
        status.azureConfigured
          ? "Connected, waiting for data"
          : "Azure not configured"

      connection.className =
        "connection-status"
    }

    if (latest) {
      $("temperature").textContent =
        fmt(latest.temperature, 1)

      $("humidity").textContent =
        fmt(latest.humidity, 1)

      $("acceleration").textContent =
        fmt(latest.acceleration, 3)

      $("healthScore").textContent =
        latest.health_score ?? "--"

      $("armed").textContent =
        latest.armed
          ? "Armed"
          : "Disarmed"

      $("alert").textContent =
        latest.alert
          ? "Alert active"
          : "Normal"

      $("lastUpdate").textContent =
        formatTime(latest.received_at)
    }

    $("storedReadings").textContent =
      status.storedReadings ?? 0

    drawLineChart(
      $("environmentChart"),
      history,
      [
        {
          key: "temperature",
          label: "Temperature"
        },
        {
          key: "humidity",
          label: "Humidity"
        }
      ]
    )

    drawLineChart(
      $("accelerationChart"),
      history,
      [
        {
          key: "acceleration",
          label: "Acceleration"
        }
      ]
    )

    const alertsEl = $("alerts")

    if (!alerts.length) {
      alertsEl.innerHTML =
        '<p class="muted">No alert events stored.</p>'
    } else {
      alertsEl.innerHTML =
        alerts.map((a) => `
          <div class="alert-item">
            <span>Impact / alert detected</span>
            <time>
              ${formatTime(a.received_at)}
            </time>
          </div>
        `).join("")
    }
  } catch (err) {
    const connection =
      $("connectionPill")

    connection.textContent =
      "Dashboard API unavailable"

    connection.className =
      "connection-status error"

    console.error(err)
  }
}

loadDashboard()

setInterval(
  loadDashboard,
  3000
)

window.addEventListener(
  "resize",
  () => setTimeout(
    loadDashboard,
    100
  )
)