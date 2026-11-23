import { createWidgetCard, getValueAtPath, mergeWidgetConfig } from './widget-base.js';

const STATUS_COLORS = {
  online: { badge: 'bg-success/10 text-success', dot: 'bg-success' },
  success: { badge: 'bg-success/10 text-success', dot: 'bg-success' },
  active: { badge: 'bg-success/10 text-success', dot: 'bg-success' },
  offline: { badge: 'bg-danger/10 text-danger', dot: 'bg-danger' },
  error: { badge: 'bg-danger/10 text-danger', dot: 'bg-danger' },
  warning: { badge: 'bg-warning/10 text-warning', dot: 'bg-warning' },
  pending: { badge: 'bg-info/10 text-info', dot: 'bg-info' },
  danger: { badge: 'bg-danger/10 text-danger', dot: 'bg-danger' },
  info: { badge: 'bg-info/10 text-info', dot: 'bg-info' },
  primary: { badge: 'bg-primary/10 text-primary', dot: 'bg-primary' },
  neutral: { badge: 'bg-muted text-muted-foreground', dot: 'bg-muted-foreground' },
};

export function mountStatusWidget(container, config = {}) {
  if (!container) return;
  const options = mergeWidgetConfig({
    title: 'Status', icon: 'activity', color: 'success', value: 'Online', detail: '',
    apiUrl: '', valuePath: 'status', detailPath: 'detail', pollIntervalMs: 5000,
  }, config);
  const { body } = createWidgetCard(container, { kind: 'Status', ...options });
  body.innerHTML = `
    <div class="flex items-center gap-2 min-h-6">
      <span class="h-2.5 w-2.5 rounded-full" data-status-dot></span>
      <span class="text-sm font-medium" data-status-value></span>
      <span class="text-xs text-muted-foreground ml-auto" data-status-detail></span>
    </div>`;
  const valueElement = body.querySelector('[data-status-value]');
  const detailElement = body.querySelector('[data-status-detail]');
  const dotElement = body.querySelector('[data-status-dot]');

  function render(value, detail = '') {
    const label = typeof value === 'boolean' ? (value ? 'Online' : 'Offline') : String(value ?? 'Unknown');
    const palette = STATUS_COLORS[label.toLowerCase()] || STATUS_COLORS[options.color] || STATUS_COLORS.online;
    valueElement.textContent = label;
    valueElement.className = `text-sm font-medium ${palette.badge.split(' ').at(-1)}`;
    dotElement.className = `h-2.5 w-2.5 rounded-full ${palette.dot}`;
    detailElement.textContent = detail || '';
  }

  render(options.value, options.detail);
  if (!options.apiUrl) return;

  const interval = Math.max(500, Number(options.pollIntervalMs) || 5000);
  let requestInProgress = false;
  async function refresh() {
    if (requestInProgress) return;
    requestInProgress = true;
    try {
      const response = await fetch(options.apiUrl, { headers: { Accept: 'application/json' } });
      if (!response.ok) throw new Error(`Status request failed (${response.status})`);
      const payload = await response.json();
      render(getValueAtPath(payload, options.valuePath), getValueAtPath(payload, options.detailPath));
    } catch (error) {
      console.error(`Status widget (${options.title}):`, error);
    } finally {
      requestInProgress = false;
    }
  }
  refresh();
  window.setInterval(refresh, interval);
}
