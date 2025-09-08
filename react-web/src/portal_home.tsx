import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './portal_default.css'
import Portal from './Portal.tsx'

createRoot(document.getElementById('root')!).render(
    <StrictMode>
        <Portal />
    </StrictMode>,
)
