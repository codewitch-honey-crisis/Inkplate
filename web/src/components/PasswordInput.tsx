import { createSignal } from "solid-js"

interface PasswordInputProps {
  name?: string
  value: string
  onInput: (value: string) => void
}

export function PasswordInput(props: PasswordInputProps) {
  const [showPassword, setShowPassword] = createSignal(false)

  return (
    <div>
      <input
        type={showPassword() ? "text" : "password"}
        name={props.name}
        value={props.value}
        onInput={(e) => props.onInput(e.currentTarget.value)}
      />
      <div class="password-toggle">
        <label>
          <input
            type="checkbox"
            checked={showPassword()}
            onChange={(e) => setShowPassword(e.currentTarget.checked)}
          />
          <span>Show password</span>
        </label>
      </div>
    </div>
  )
}
