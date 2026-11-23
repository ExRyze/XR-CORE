const COLORS = {
  primary: { badge: 'bg-primary/10 text-primary', glow: 'bg-primary/10', value: 'text-primary', bar: 'bg-primary', dot: 'bg-primary' },
  success: { badge: 'bg-success/10 text-success', glow: 'bg-success/10', value: 'text-success', bar: 'bg-success', dot: 'bg-success' },
  warning: { badge: 'bg-warning/10 text-warning', glow: 'bg-warning/10', value: 'text-warning', bar: 'bg-warning', dot: 'bg-warning' },
  danger: { badge: 'bg-danger/10 text-danger', glow: 'bg-danger/10', value: 'text-danger', bar: 'bg-danger', dot: 'bg-danger' },
  info: { badge: 'bg-info/10 text-info', glow: 'bg-info/10', value: 'text-info', bar: 'bg-info', dot: 'bg-info' },
  neutral: { badge: 'bg-muted text-muted-foreground', glow: 'bg-muted/40', value: 'text-foreground', bar: 'bg-muted-foreground', dot: 'bg-muted-foreground' },
};

export function mergeWidgetConfig(defaults, config) {
  const definedOptions = Object.fromEntries(
    Object.entries(config).filter(([, value]) => value !== undefined && value !== '')
  );
  return { ...defaults, ...definedOptions };
}

export function createWidgetCard(container, { kind, title, icon = 'activity', color = 'primary' }) {
  const palette = COLORS[color] || COLORS.primary;
  const card = document.createElement('article');
  card.className = 'bg-card border border-border rounded-2xl p-5 shadow-lg relative overflow-hidden';
  card.innerHTML = `
    <div class="absolute -right-8 -top-10 w-32 h-32 rounded-full blur-2xl ${palette.glow}"></div>
    <div class="relative flex items-start justify-between">
      <div>
        <p class="text-xs text-muted-foreground">${escapeHtml(kind)}</p>
        <h3 class="font-semibold text-foreground mt-1" data-widget-title></h3>
      </div>
      <span class="p-2.5 rounded-xl ${palette.badge}"><i class="w-5 h-5" data-widget-icon></i></span>
    </div>
    <div class="relative mt-5" data-widget-body></div>`;
  card.querySelector('[data-widget-title]').textContent = title;
  card.querySelector('[data-widget-icon]').setAttribute('data-lucide', icon);
  container.replaceChildren(card);
  window.lucide?.createIcons();
  return { card, body: card.querySelector('[data-widget-body]'), palette };
}

export async function postWidgetValue(url, value) {
  if (!url) return;
  const response = await fetch(url, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'X-Requested-With': 'XMLHttpRequest',
    },
    body: JSON.stringify({ value }),
  });
  if (!response.ok) throw new Error(`Widget update failed (${response.status})`);
}

export function getValueAtPath(object, path) {
  return String(path || '').split('.').filter(Boolean).reduce((value, key) => value?.[key], object);
}

function escapeHtml(value) {
  return String(value ?? '').replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  })[character]);
}
