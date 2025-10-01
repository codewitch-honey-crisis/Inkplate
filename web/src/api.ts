import type { ConfigData, TimezoneData } from "./types"

export const fetchPortal = async (): Promise<ConfigData> => {
  const portal = import.meta.env.DEV ? "./api/portal.json" : "./api/portal.clasp"
  const res = await fetch(portal)
  return res.json()
}

export const fetchTimezones = async (): Promise<TimezoneData[]> => {
  const res = await fetch("./api/timezones.json")
  return res.json()
}
