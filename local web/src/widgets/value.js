import { createWidgetCard, mergeWidgetConfig } from './widget-base.js';

const DEFAULTS = {
  apiUrl: '',
  title: 'Variable',
  label: 'Value',
  unit: '',
  pollIntervalMs: 2000,
  decimals: 2,
  valuePath: 'value',
  color: 'primary',
  icon: 'activity',
};

export function mountValueWidget(container, options = {}) {
  if (!container) return;

  const config = mergeWidgetConfig(DEFAULTS, options);
  const pollIntervalMs = Math.max(500, Number(config.pollIntervalMs) || DEFAULTS.pollIntervalMs);
  const decimals = Math.max(0, Math.min(6, Number(config.decimals) || 0));
  if (!config.apiUrl) {
    console.error('Value widget requires an apiUrl.', container);
    return;
  }

  const { body, palette } = createWidgetCard(container, {
    kind: config.label || 'Value',
    title: config.title,
    icon: config.icon,
    color: config.color,
  });
  body.innerHTML = `
    <p class="text-3xl font-bold ${palette.value}">
      <span data-value>--</span><span class="text-base font-medium text-muted-foreground ml-1" data-unit></span>
    </p>`;
  const valueElement = body.querySelector('[data-value]');
  const unitElement = body.querySelector('[data-unit]');
  unitElement.textContent = config.unit;

  let requestInProgress = false;
  async function refresh() {
    if (requestInProgress) return;
    requestInProgress = true;
    try {
      const response = await fetch(config.apiUrl, { headers: { Accept: 'application/json' } });
      if (!response.ok) throw new Error(`Value request failed (${response.status})`);
      const payload = await response.json();
      const value = config.valuePath.split('.').reduce((current, key) => current?.[key], payload);
      if (payload.valid === false || value == null) {
        valueElement.textContent = '--';
        return;
      }
      if (!Number.isFinite(Number(value))) throw new Error(`Value at "${config.valuePath}" is invalid`);

      valueElement.textContent = Number(value).toFixed(decimals);
      unitElement.textContent = config.unit || payload.unit || '';
    } catch (error) {
      console.error(`Value widget (${config.title}):`, error);
    } finally {
      requestInProgress = false;
    }
  }

  refresh();
  window.setInterval(refresh, pollIntervalMs);
}
