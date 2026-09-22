// Documentation : la recherche de la barre d'en-tête, et le filtre des cas du cahier de test.
// Sans script, tout reste lisible — la recherche et le filtre sont un confort, pas un contenu.
(() => {
  const root = document.body.dataset.root || '.';

  // -- recherche : `search.json` n'est chargé qu'à la première frappe ----------------------------
  const form = document.querySelector('.topbar .search');
  if (form) {
    const input = form.querySelector('input');
    const results = form.querySelector('.results');
    let index = null;
    const fold = (text) => text.toLowerCase().normalize('NFD').replace(/[̀-ͯ]/g, '');
    const show = () => {
      const needle = fold(input.value.trim());
      if (!index || needle.length < 2) { results.hidden = true; return; }
      const hits = [];
      index.forEach((entry) => {
        const inTitle = fold(entry.t).includes(needle);
        const heading = entry.h.find((h) => fold(h).includes(needle));
        const at = fold(entry.x).indexOf(needle);
        if (!inTitle && !heading && at < 0) return;
        const excerpt = at >= 0 ? entry.x.slice(Math.max(0, at - 40), at + 80) : (heading || '');
        hits.push({ entry, excerpt, score: inTitle ? 0 : heading ? 1 : 2 });
      });
      hits.sort((a, b) => a.score - b.score);
      results.replaceChildren(...hits.slice(0, 12).map(({ entry, excerpt }) => {
        const link = document.createElement('a');
        link.href = root + '/' + entry.u;
        const title = document.createElement('strong');
        title.textContent = entry.t;
        const text = document.createElement('span');
        text.textContent = excerpt;
        link.append(title, text);
        return link;
      }));
      if (!hits.length) results.textContent = 'Rien trouvé.';
      results.hidden = false;
    };
    input.addEventListener('input', () => {
      if (index) { show(); return; }
      fetch(root + '/search.json').then((r) => r.json()).then((data) => { index = data; show(); })
        .catch(() => { index = []; });
    });
    form.addEventListener('submit', (event) => {
      event.preventDefault();
      const first = results.querySelector('a');
      if (first) window.location.href = first.href;
    });
    document.addEventListener('click', (event) => { if (!form.contains(event.target)) results.hidden = true; });
  }

  // -- cahier de test : filtrer les cartes de cas ------------------------------------------------
  document.querySelectorAll('[data-filter-cases]').forEach((filters) => {
    const cases = Array.from(document.querySelectorAll('section.case'));
    const search = filters.querySelector('input');
    const select = filters.querySelector('select');
    const count = filters.querySelector('.count');
    const apply = () => {
      const needle = search.value.trim().toLowerCase();
      let shown = 0;
      cases.forEach((item) => {
        const visible = (!needle || item.textContent.toLowerCase().includes(needle))
          && (!select.value || item.dataset.crit === select.value);
        item.hidden = !visible;
        if (visible) shown += 1;
      });
      count.textContent = shown + ' / ' + cases.length + ' cas';
    };
    search.addEventListener('input', apply);
    select.addEventListener('change', apply);
  });
})();
