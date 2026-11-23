import { createWidgetCard, mergeWidgetConfig, postWidgetValue } from './widget-base.js';

export function mountSwitchWidget(container, config = {}) {
  if (!container) return;
  const options = mergeWidgetConfig({
    title: 'Switch', icon: 'lightbulb', color: 'primary', value: false,
    onLabel: 'On', offLabel: 'Off', apiUrl: '',
  }, config);
  const { body, palette } = createWidgetCard(container, { kind: 'Switch', ...options });
  body.innerHTML = `
    <div class="flex items-center justify-between gap-4">
      <span class="text-sm font-medium" data-switch-label></span>
      <button type="button" role="switch" aria-checked="false"
        class="relative h-7 w-12 rounded-full transition-colors focus:outline-none focus:ring-2 focus:ring-primary/60"
        aria-label="Toggle ${String(options.title).replace(/["<>]/g, '')}">
        <span class="absolute top-1 h-5 w-5 rounded-full bg-foreground shadow transition-all" data-switch-knob></span>
      </button>
    </div>`;
  const button = body.querySelector('button');
  const label = body.querySelector('[data-switch-label]');
  const knob = body.querySelector('[data-switch-knob]');
  let value = options.value === true || String(options.value).toLowerCase() === 'true';

  function render() {
    button.setAttribute('aria-checked', String(value));
    button.classList.toggle(palette.value.replace('text-', 'bg-'), value);
    button.classList.toggle('bg-muted', !value);
    knob.classList.toggle('right-1', value);
    knob.classList.toggle('left-1', !value);
    label.textContent = value ? options.onLabel : options.offLabel;
    label.className = `text-sm font-medium ${value ? palette.value : 'text-muted-foreground'}`;
  }

  button.addEventListener('click', async () => {
    const previous = value;
    value = !value;
    render();
    button.disabled = true;
    try {
      await postWidgetValue(options.apiUrl, value);
    } catch (error) {
      value = previous;
      render();
      console.error(`Switch widget (${options.title}):`, error);
    } finally {
      button.disabled = false;
    }
  });
  render();
}
