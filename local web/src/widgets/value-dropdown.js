import { createWidgetCard, mergeWidgetConfig, postWidgetValue } from './widget-base.js';

export function mountValueDropdownWidget(container, config = {}) {
    if (!container) return;

    const options = mergeWidgetConfig({
        title: 'Value dropdown',
        icon: 'fan',
        color: 'info',
        choices: ['Option 1', 'Option 2'],
        value: '',
        apiUrl: '',
    }, config);

    const { body } = createWidgetCard(container, { kind: 'Value dropdown', ...options });
    const select = document.createElement('select');
    select.className = 'w-full bg-background border border-border rounded-xl px-3 py-2.5 text-sm text-foreground';

    const choices = Array.isArray(options.choices) ? options.choices : [];
    choices.forEach((choice) => {
        const item = document.createElement('option');
        if (typeof choice === 'object' && choice !== null) {
            item.value = String(choice.value);
            item.textContent = String(choice.label ?? choice.value);
        } else {
            item.value = String(choice);
            item.textContent = String(choice);
        }
        select.appendChild(item);
    });

    if (options.value !== '') select.value = String(options.value);
    body.appendChild(select);

    let currentValue = select.value;

    select.addEventListener('change', async () => {
        const previous = currentValue;
        const selected = select.value;
        select.disabled = true;

        try {
            await postWidgetValue(options.apiUrl, selected);
            currentValue = selected;
        } catch (error) {
            console.error(`Value dropdown widget (${options.title}):`, error);
            select.value = previous;
        } finally {
            select.disabled = false;
        }
    });
}
