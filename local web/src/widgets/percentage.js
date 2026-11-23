import { createWidgetCard, getValueAtPath, mergeWidgetConfig } from './widget-base.js';

const DEFAULTS = {
  apiUrl: '',
  title: 'Percentage',
  label: 'Percentage',
  value: 0,
  valuePath: 'value',
  min: 0,
  max: 100,
  unit: '%',
  variant: 'full',
  detail: '',
  pollIntervalMs: 2000,
  decimals: 0,
  color: 'primary',
  icon: 'gauge',
};

const VARIANTS = new Set(['semicircle', 'three-quarter', 'full', 'line']);

export function mountPercentageWidget(container, config = {}) {
  if (!container) return;
  const options = mergeWidgetConfig(DEFAULTS, config);
  options.min = Number(options.min);
  options.max = Number(options.max);
  if (!Number.isFinite(options.min)) options.min = 0;
  if (!Number.isFinite(options.max) || options.max <= options.min) options.max = options.min + 100;
  options.decimals = Math.max(0, Math.min(6, Number(options.decimals) || 0));
  options.variant = VARIANTS.has(options.variant) ? options.variant : 'full';

  const { body, palette } = createWidgetCard(container, {
    kind: options.label,
    title: options.title,
    icon: options.icon,
    color: options.color,
  });

  if (options.variant === 'line') {
    body.innerHTML = `
      <div class="flex items-baseline justify-between gap-2">
        <p class="text-3xl font-bold ${palette.value}"><span data-value>--</span><span class="text-base font-medium text-muted-foreground ml-1" data-unit></span></p>
        <p class="text-xs text-muted-foreground text-right" data-detail></p>
      </div>
      <div class="mt-4 h-2.5 rounded-full bg-muted overflow-hidden" role="progressbar" aria-valuemin="${options.min}" aria-valuemax="${options.max}" aria-valuenow="${options.value}">
        <div class="h-full rounded-full transition-all duration-500 ${palette.bar}" data-progress></div>
      </div>`;
  } else {
    const arcMarkup = getArcMarkup(options.variant, palette.value);
    const valueMarkup = `<p class="text-3xl font-bold ${palette.value}"><span data-value>--</span><span class="text-base font-medium text-muted-foreground ml-1" data-unit></span></p><p class="text-[10px] text-muted-foreground" data-detail></p>`;
    body.innerHTML = `<div class="relative mx-auto aspect-square w-full max-w-[200px]">${arcMarkup}<div class="absolute inset-0 flex flex-col items-center justify-center">${valueMarkup}</div></div>`;
  }

  const valueElement = body.querySelector('[data-value]');
  const unitElement = body.querySelector('[data-unit]');
  const detailElement = body.querySelector('[data-detail]');
  const progressElement = body.querySelector('[data-progress]');
  const progressbar = body.querySelector('[role="progressbar"]');
  const progressShape = body.querySelector('[data-progress-shape]');
  unitElement.textContent = options.unit;
  detailElement.textContent = options.detail;

  function render(rawValue, detail = options.detail) {
    const value = Number(rawValue);
    if (!Number.isFinite(value)) throw new Error('Percentage value is invalid');
    const ratio = Math.max(0, Math.min(1, (value - options.min) / (options.max - options.min)));
    valueElement.textContent = value.toFixed(options.decimals);
    if (detailElement) detailElement.textContent = detail ?? '';
    if (progressbar) progressbar.setAttribute('aria-valuenow', String(value));
    if (progressElement) progressElement.style.width = `${ratio * 100}%`;
    if (progressShape) {
      const arcLength = options.variant === 'three-quarter' ? 75 : 100;
      progressShape.setAttribute('stroke-dasharray', `${ratio * arcLength} 100`);
    }
  }

  render(options.value);
  if (!options.apiUrl) return;

  const pollIntervalMs = Math.max(500, Number(options.pollIntervalMs) || DEFAULTS.pollIntervalMs);
  let requestInProgress = false;
  async function refresh() {
    if (requestInProgress) return;
    requestInProgress = true;
    try {
      const response = await fetch(options.apiUrl, { headers: { Accept: 'application/json' } });
      if (!response.ok) throw new Error(`Percentage request failed (${response.status})`);
      const payload = await response.json();
      render(getValueAtPath(payload, options.valuePath), payload.detail ?? options.detail);
      if (!options.unit && payload.unit) unitElement.textContent = payload.unit;
    } catch (error) {
      console.error(`Percentage widget (${options.title}):`, error);
    } finally {
      requestInProgress = false;
    }
  }
  refresh();
  window.setInterval(refresh, pollIntervalMs);
}

function getArcMarkup(variant, colorClass) {
  if (variant === 'semicircle') {
    return `<svg class="block w-full overflow-visible" viewBox="0 0 100 60" role="img" aria-label="Percentage gauge">
      <path d="M 10 50 A 40 40 0 0 1 90 50" pathLength="100" fill="none" stroke="currentColor" stroke-width="8" stroke-linecap="round" class="text-muted" />
      <path d="M 10 50 A 40 40 0 0 1 90 50" pathLength="100" fill="none" stroke="currentColor" stroke-width="8" stroke-linecap="round" class="gauge-circle ${colorClass}" data-progress-shape />
    </svg>`;
  }

  const dashArray = variant === 'three-quarter' ? '75 25' : '100';
  const rotation = variant === 'three-quarter' ? 'rotate(135 50 50)' : 'rotate(-90 50 50)';
  return `<svg class="block w-full h-full overflow-visible" viewBox="0 0 100 100" role="img" aria-label="Percentage gauge">
    <g transform="${rotation}">
      <circle cx="50" cy="50" r="40" pathLength="100" fill="none" stroke="currentColor" stroke-width="8" stroke-linecap="round" stroke-dasharray="${dashArray}" class="text-muted" />
      <circle cx="50" cy="50" r="40" pathLength="100" fill="none" stroke="currentColor" stroke-width="8" stroke-linecap="round" stroke-dasharray="0 100" class="gauge-circle ${colorClass}" data-progress-shape />
    </g>
  </svg>`;
}
