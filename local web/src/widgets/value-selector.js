import { createWidgetCard, mergeWidgetConfig, postWidgetValue } from './widget-base.js';

export function mountValueSelectorWidget(container, config = {}) {
    if (!container) return;

    const options = mergeWidgetConfig({
        title: 'Value selector',
        icon: 'fan',
        color: 'info',
        choices: ['Option 1', 'Option 2'],
        value: '',
        apiUrl: '',
    }, config);

    const { body } = createWidgetCard(container, { kind: 'Value selector', ...options });

    const wrapper = document.createElement('div');
    wrapper.className = 'flex gap-2';

    const choices = Array.isArray(options.choices) ? options.choices : [];
    const initialValue = String(options.value ?? '');

    const buttons = choices.map((choice) => {
        const item = document.createElement('button');
        item.type = 'button';
        item.className = 'flex grow items-center justify-center rounded-xl border px-2 py-2 text-xs font-medium transition-colors';

        let rawValue = '';
        let label = '';

        if (typeof choice === 'object' && choice !== null) {
            rawValue = String(choice.value ?? choice.label ?? '');
            label = String(choice.label ?? choice.value ?? '');
        } else {
            rawValue = String(choice);
            label = String(choice);
        }

        item.dataset.value = rawValue;
        item.dataset.selectedValue = initialValue;
        item.textContent = label;

        const isActive = rawValue === initialValue;
        item.classList.toggle('border-primary', isActive);
        item.classList.toggle('bg-primary/10', isActive);
        item.classList.toggle('text-primary', isActive);
        item.classList.toggle('border-border', !isActive);
        item.classList.toggle('bg-background', !isActive);
        item.classList.toggle('text-foreground', !isActive);

        item.addEventListener('click', async () => {
            const previousValue = buttons.find((button) => button.classList.contains('border-primary'))?.dataset.value ?? initialValue;
            const selectedValue = rawValue;

            buttons.forEach((button) => {
                const isSelected = button.dataset.value === selectedValue;
                button.classList.toggle('border-primary', isSelected);
                button.classList.toggle('bg-primary/10', isSelected);
                button.classList.toggle('text-primary', isSelected);
                button.classList.toggle('border-border', !isSelected);
                button.classList.toggle('bg-background', !isSelected);
                button.classList.toggle('text-foreground', !isSelected);
                button.dataset.selectedValue = selectedValue;
            });

            item.disabled = true;
            try {
                await postWidgetValue(options.apiUrl, selectedValue);
            } catch (error) {
                console.error(`Value selector widget (${options.title}):`, error);
                buttons.forEach((button) => {
                    const isSelected = button.dataset.value === previousValue;
                    button.classList.toggle('border-primary', isSelected);
                    button.classList.toggle('bg-primary/10', isSelected);
                    button.classList.toggle('text-primary', isSelected);
                    button.classList.toggle('border-border', !isSelected);
                    button.classList.toggle('bg-background', !isSelected);
                    button.classList.toggle('text-foreground', !isSelected);
                    button.dataset.selectedValue = previousValue;
                });
            } finally {
                item.disabled = false;
            }
        });

        return item;
    });

    buttons.forEach((button) => wrapper.appendChild(button));
    body.appendChild(wrapper);
}
