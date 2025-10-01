export type TimezoneData = {
  value: string
  offset: string
}

export type ConfigData = {
  ap_list: string[]
  pass?: string
}

export type FormState = {
  ssid: string
  pass: string
  loc: string
  units: string
  tz: string
}
