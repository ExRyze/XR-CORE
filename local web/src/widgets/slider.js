import { createWidgetCard, mergeWidgetConfig, postWidgetValue } from './widget-base.js';

export function mountSliderWidget(container, config = {}) {
    if (!container) return;

    const options = mergeWidgetConfig({
        title: 'Slider',
        icon: 'sliders-horizontal',
        color: 'primary',
        value: 0,
        min: 0,
        max: 100,
        step: 1,
        unit: '',
        decimals: 0,
        apiUrl: '',
    }, config);

    options.value = Number.isFinite(Number(options.value)) ? Number(options.value) : 0;
    options.min = Number.isFinite(Number(options.min)) ? Number(options.min) : 0;
    options.max = Number.isFinite(Number(options.max)) ? Number(options.max) : 100;
    options.step = Math.abs(Number(options.step)) || 1;
    options.decimals = Math.max(0, Math.min(6, Number(options.decimals) || 0));

    const { body, palette } = createWidgetCard(container, { kind: 'Slider', ...options });
    body.innerHTML = `
    <div class="space-y-3">
      <div class="flex items-center justify-between gap-3 text-sm">
        <span class="text-muted-foreground">Value</span>
        <span class="font-semibold ${palette.value}" data-slider-value></span>
      </div>
      <input type="range" data-slider-input
        class="w-full accent-primary cursor-pointer"
        aria-label="${String(options.title).replace(/['"<>]/g, '')} slider" />
    </div>
  `;

    const input = body.querySelector('[data-slider-input]');
    const valueEl = body.querySelector('[data-slider-value]');
    let currentValue = options.value;

    const render = () => {
        input.min = String(options.min);
        input.max = String(options.max);
        input.step = String(options.step);
        input.value = String(currentValue);
        const formatted = `${Number(currentValue).toFixed(options.decimals)}${options.unit}`;
        valueEl.textContent = formatted;
    };

    input.addEventListener('input', () => {
        const parsed = Number.parseFloat(input.value);
        if (!Number.isFinite(parsed)) return;
        currentValue = parsed;
        render();
    });

    input.addEventListener('change', async () => {
        const previousValue = currentValue;
        const parsed = Number.parseFloat(input.value);
        if (!Number.isFinite(parsed)) {
            currentValue = previousValue;
            render();
            return;
        }

        currentValue = Math.min(options.max, Math.max(options.min, parsed));
        input.disabled = true;
        try {
            await postWidgetValue(options.apiUrl, currentValue);
        } catch (error) {
            currentValue = previousValue;
            console.error(`Slider widget (${options.title}):`, error);
        } finally {
            input.disabled = false;
            render();
        }
    });

    render();
}
