import { createWidgetCard, mergeWidgetConfig, postWidgetValue } from './widget-base.js';

export function mountPushButtonWidget(container, config = {}) {
    if (!container) return;

    const options = mergeWidgetConfig({
        title: 'Push button',
        icon: 'zap',
        color: 'primary',
        label: 'Run',
        value: true,
        apiUrl: '',
        busyLabel: 'Sending...',
    }, config);

    const { body, palette } = createWidgetCard(container, { kind: 'Push button', ...options });

    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'relative mx-auto flex h-20 w-20 items-center justify-center rounded-full border-2 text-xs font-bold transition-all duration-150 focus:outline-none focus:ring-2 focus:ring-primary/60';
    button.setAttribute('aria-label', String(options.title));
    button.setAttribute('aria-pressed', 'false');

    const label = document.createElement('span');
    label.className = 'pointer-events-none';
    label.textContent = String(options.label || 'ON');

    button.appendChild(label);
    body.appendChild(button);

    let active = false;
    let isSending = false;

    const render = () => {
        const isActive = active;
        button.setAttribute('aria-pressed', String(isActive));
        button.classList.toggle('border-primary', isActive);
        button.classList.toggle('bg-primary/15', isActive);
        button.classList.toggle('text-primary', isActive);
        button.classList.toggle('border-border', !isActive);
        button.classList.toggle('bg-muted', !isActive);
        button.classList.toggle('text-foreground', !isActive);
        button.classList.toggle('shadow-lg', isActive);
        button.classList.toggle('scale-105', isActive);
        label.textContent = isActive ? 'ON' : 'OFF';
    };

    const sendState = async (state) => {
        if (!options.apiUrl || isSending) return;
        isSending = true;
        try {
            await postWidgetValue(options.apiUrl, state);
        } catch (error) {
            console.error(`Push button widget (${options.title}):`, error);
        } finally {
            isSending = false;
        }
    };

    const setActiveState = async (state) => {
        active = state;
        render();
        await sendState(state);
    };

    const pressStart = () => {
        if (button.disabled) return;
        setActiveState(true);
    };

    const pressEnd = () => {
        if (button.disabled) return;
        setActiveState(false);
    };

    button.addEventListener('pointerdown', pressStart);
    button.addEventListener('pointerup', pressEnd);
    button.addEventListener('pointerleave', pressEnd);
    button.addEventListener('pointercancel', pressEnd);
    button.addEventListener('keydown', (event) => {
        if (event.key === ' ' || event.key === 'Enter') {
            event.preventDefault();
            pressStart();
        }
    });
    button.addEventListener('keyup', (event) => {
        if (event.key === ' ' || event.key === 'Enter') {
            event.preventDefault();
            pressEnd();
        }
    });

    render();
}
