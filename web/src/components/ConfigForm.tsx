import { createStore } from "solid-js/store"
import { createResource, createEffect, For } from "solid-js"
import { fetchPortal, fetchTimezones } from "../api"
import type { FormState } from "../types"
import { PasswordInput } from "./PasswordInput"

export function ConfigForm() {
  const [portalData] = createResource(fetchPortal)
  const [tzData] = createResource(fetchTimezones)

  const [form, setForm] = createStore<FormState>({
    ssid: "",
    pass: "",
    loc: "",
    units: "auto",
    tz: "",
  })

  let initialized = false

  const ssids = () => portalData() ? [...new Set(portalData()!.ap_list)] : []

  createEffect(() => {
    const data = portalData()
    if (data && !initialized) {
      initialized = true
      if (!form.ssid && ssids().length > 0) setForm("ssid", ssids()[0])
      if (data.pass) setForm("pass", data.pass)
    }
  })

  createEffect(() => {
    const tzs = tzData()
    if (tzs && tzs.length > 0 && !form.tz) {
      setForm("tz", tzs[0].offset)
    }
  })

  const submit = (e: SubmitEvent) => {
    e.preventDefault()
    const q = new URLSearchParams({
      ssid: form.ssid || ssids()[0] || "",
      pass: form.pass || portalData()?.pass || "",
      location: form.loc,
      units: form.units,
      tz: form.tz || tzData()?.[0]?.offset || "",
      Save: "Save",
    })
    window.location.href = `?${q.toString()}`
  }

  return (
    <form method="get" onSubmit={submit}>
      <div class="form-group">
        <label>SSID</label>
        <select value={form.ssid} onChange={(e) => setForm("ssid", e.currentTarget.value)}>
          <For each={ssids()}>
            {(ssid) => <option value={ssid}>{ssid}</option>}
          </For>
        </select>
      </div>

      <div class="form-group">
        <label>Password</label>
        <PasswordInput
          name="pass"
          value={form.pass}
          onInput={(value) => setForm("pass", value)}
        />
      </div>

      <div class="form-group">
        <label>Location (geo coords or city/zip/postal US/UK/CA)</label>
        <input
          type="text"
          name="location"
          value={form.loc}
          onInput={(e) => setForm("loc", e.currentTarget.value)}
        />
      </div>

      <div class="form-group">
        <label>Units</label>
        <select value={form.units} onChange={(e) => setForm("units", e.currentTarget.value)}>
          <option value="auto">Automatic</option>
          <option value="metric">Metric</option>
          <option value="imperial">Imperial</option>
        </select>
      </div>

      <div class="form-group">
        <label>Timezone</label>
        <select value={form.tz} onChange={(e) => setForm("tz", e.currentTarget.value)}>
          <For each={tzData()}>
            {(tz) => <option value={tz.offset}>{tz.value}</option>}
          </For>
        </select>
      </div>

      <button type="submit">Save</button>
    </form>
  )
}
