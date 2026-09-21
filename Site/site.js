// Filtres des tables : une recherche plein texte et des listes qui lisent les data-* des lignes.
// Sans script, la table reste entière et lisible — le filtre est un confort, pas un contenu.
document.querySelectorAll('.filters').forEach((filters) => {
  const table = document.querySelector('table.' + filters.dataset.filterTable);
  if (!table) return;
  const rows = Array.from(table.tBodies[0].rows);
  const search = filters.querySelector('input[type=search]');
  const selects = Array.from(filters.querySelectorAll('select'));
  const count = filters.querySelector('.count');

  const apply = () => {
    const needle = (search ? search.value : '').trim().toLowerCase();
    let shown = 0;
    rows.forEach((row) => {
      const matchesText = !needle || row.textContent.toLowerCase().includes(needle);
      const matchesKeys = selects.every((select) => !select.value || row.dataset[select.dataset.key] === select.value);
      row.hidden = !(matchesText && matchesKeys);
      if (!row.hidden) shown += 1;
    });
    if (count) count.textContent = shown + ' / ' + rows.length + ' lignes';
  };
  if (search) search.addEventListener('input', apply);
  selects.forEach((select) => select.addEventListener('change', apply));
});
