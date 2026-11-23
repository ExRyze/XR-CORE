import { createWidgetCard, mergeWidgetConfig, postWidgetValue } from './widget-base.js';

export function mountStepperWidget(container, config = {}) {
  if (!container) return;
  const options = mergeWidgetConfig({
    title: 'Stepper', icon: 'sliders-horizontal', color: 'primary', value: 0,
    min: -Infinity, max: Infinity, step: 1, unit: '', apiUrl: '', decimals: 0,
  }, config);
  options.value = Number(options.value) || 0;
  options.step = Math.abs(Number(options.step)) || 1;
  options.min = Number.isFinite(Number(options.min)) ? Number(options.min) : -Infinity;
  options.max = Number.isFinite(Number(options.max)) ? Number(options.max) : Infinity;
  options.decimals = Math.max(0, Math.min(6, Number(options.decimals) || 0));
  const { body } = createWidgetCard(container, { kind: 'Stepper', ...options });
  body.innerHTML = `
    <div class="flex items-center justify-between gap-3">
      <button type="button" data-step="-1" aria-label="Decrease ${options.title}"
        class="h-9 w-9 rounded-lg border border-border text-lg hover:bg-muted disabled:opacity-40">−</button>
      <span class="text-center"><b class="text-2xl text-foreground" data-step-value></b><span class="text-sm text-muted-foreground ml-1" data-step-unit></span></span>
      <button type="button" data-step="1" aria-label="Increase ${options.title}"
        class="h-9 w-9 rounded-lg border border-border text-lg hover:bg-muted disabled:opacity-40">+</button>
    </div>`;
  const valueElement = body.querySelector('[data-step-value]');
  const unitElement = body.querySelector('[data-step-unit]');
  const buttons = [...body.querySelectorAll('button')];
  let value = options.value;
  let requestInProgress = false;

  function render() {
    valueElement.textContent = value.toFixed(options.decimals);
    unitElement.textContent = options.unit;
    buttons[0].disabled = value <= options.min;
    buttons[1].disabled = value >= options.max;
  }

  buttons.forEach((button) => button.addEventListener('click', async () => {
    if (requestInProgress) return;
    const previous = value;
    value = Math.min(options.max, Math.max(options.min, value + Number(button.dataset.step) * options.step));
    render();
    if (value === previous) return;
    requestInProgress = true;
    buttons.forEach((item) => { item.disabled = true; });
    try {
      await postWidgetValue(options.apiUrl, value);
    } catch (error) {
      value = previous;
      console.error(`Stepper widget (${options.title}):`, error);
    } finally {
      requestInProgress = false;
      render();
    }
  }));
  render();
}
