import { createWidgetCard, mergeWidgetConfig, postWidgetValue } from './widget-base.js';

export function mountValueInputWidget(container, config = {}) {
    if (!container) return;

    const options = mergeWidgetConfig({
        title: 'Value input',
        icon: 'sliders-horizontal',
        color: 'primary',
        value: 0,
        min: -Infinity,
        max: Infinity,
        step: 1,
        unit: '',
        decimals: 0,
        apiUrl: '',
    }, config);

    options.value = Number.isFinite(Number(options.value)) ? Number(options.value) : 0;
    options.step = Math.abs(Number(options.step)) || 1;
    options.min = Number.isFinite(Number(options.min)) ? Number(options.min) : -Infinity;
    options.max = Number.isFinite(Number(options.max)) ? Number(options.max) : Infinity;
    options.decimals = Math.max(0, Math.min(6, Number(options.decimals) || 0));

    const { body } = createWidgetCard(container, { kind: 'Value input', ...options });
    body.innerHTML = `
    <div class="flex items-center gap-2">
      <input
        type="number"
        data-value-input
        class="w-full bg-background border border-border rounded-xl px-3 py-2.5 text-sm text-foreground focus:outline-none focus:ring-2 focus:ring-primary"
      />
      <span class="text-sm text-muted-foreground whitespace-nowrap" data-value-unit></span>
    </div>
  `;

    const input = body.querySelector('[data-value-input]');
    const unit = body.querySelector('[data-value-unit]');
    let currentValue = options.value;
    let isSubmitting = false;

    const render = () => {
        input.value = Number.isFinite(currentValue) ? currentValue.toFixed(options.decimals) : '0';
        input.step = String(options.step);
        input.min = Number.isFinite(options.min) ? String(options.min) : '';
        input.max = Number.isFinite(options.max) ? String(options.max) : '';
        unit.textContent = options.unit;
    };

    const clamp = (value) => Math.min(options.max, Math.max(options.min, value));

    input.addEventListener('change', async () => {
        const previousValue = currentValue;
        const parsedValue = Number.parseFloat(input.value);
        currentValue = Number.isFinite(parsedValue) ? clamp(parsedValue) : previousValue;
        render();

        if (currentValue === previousValue) return;
        if (isSubmitting) return;

        isSubmitting = true;
        input.disabled = true;
        try {
            await postWidgetValue(options.apiUrl, currentValue);
        } catch (error) {
            currentValue = previousValue;
            console.error(`Value input widget (${options.title}):`, error);
        } finally {
            isSubmitting = false;
            input.disabled = false;
            render();
        }
    });

    render();
}
