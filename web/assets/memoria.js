// Pagina de memoria: lee memoria.md del servidor, separa las entradas "## [fecha] titulo"
// y las muestra ordenadas por fecha (mas nuevas primero por defecto).
(function () {
  const els = {
    list: document.getElementById("memList"),
    search: document.getElementById("mSearch"),
    day: document.getElementById("mDay"),
    order: document.getElementById("mOrder"),
    note: document.getElementById("mNote"),
    total: document.getElementById("mTotal"),
    days: document.getElementById("mDays"),
    last: document.getElementById("mLast")
  };

  const esc = (s) => s.replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));

  // Markdown en linea: `codigo`, **negrita**, *cursiva* y [texto](url).
  function inline(text) {
    const parts = text.split(/(`[^`]*`)/g);
    return parts.map((part) => {
      if (part.startsWith("`") && part.endsWith("`") && part.length > 1) {
        return "<code>" + esc(part.slice(1, -1)) + "</code>";
      }
      return esc(part)
        .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
        .replace(/(^|[^*])\*([^*\s][^*]*)\*/g, "$1<em>$2</em>")
        .replace(/\[([^\]]+)\]\((https?:[^)\s]+)\)/g, '<a href="$2" target="_blank" rel="noopener">$1</a>');
    }).join("");
  }

  // Bloque: listas con sangria (2 espacios = 1 nivel) y parrafos.
  function block(lines) {
    let html = "";
    let depth = 0;
    let para = [];
    const flushPara = () => {
      if (para.length) { html += "<p>" + inline(para.join(" ")) + "</p>"; para = []; }
    };
    const closeTo = (d) => { while (depth > d) { html += "</li></ul>"; depth--; } };

    lines.forEach((raw) => {
      const m = raw.match(/^(\s*)[-*]\s+(.*)$/);
      if (m) {
        flushPara();
        const level = Math.floor(m[1].replace(/\t/g, "  ").length / 2) + 1;
        if (level > depth) {
          while (depth < level) { html += "<ul>"; depth++; if (depth < level) html += "<li>"; }
        } else {
          closeTo(level);
          html += "</li>";
        }
        html += "<li>" + inline(m[2]);
      } else if (raw.trim() === "") {
        closeTo(0);
        flushPara();
      } else if (depth > 0) {
        html += " " + inline(raw.trim());
      } else {
        para.push(raw.trim());
      }
    });
    closeTo(0);
    flushPara();
    return html;
  }

  function parse(md) {
    const entries = [];
    let current = null;
    md.split(/\r?\n/).forEach((line) => {
      const h = line.match(/^##\s+\[([^\]]+)\]\s*(.*)$/);
      if (h) {
        const stamp = h[1].trim();
        const dm = stamp.match(/(\d{4}-\d{2}-\d{2})(?:[ T](\d{1,2}:\d{2}))?/);
        current = {
          index: entries.length,
          stamp,
          date: dm ? dm[1] : "",
          time: dm && dm[2] ? dm[2].padStart(5, "0") : "",
          title: h[2].trim(),
          lines: []
        };
        entries.push(current);
      } else if (current && !/^---\s*$/.test(line)) {
        current.lines.push(line);
      }
    });
    entries.forEach((e) => {
      e.key = e.date + " " + (e.time || "00:00");
      e.text = (e.title + " " + e.lines.join(" ")).toLowerCase();
    });
    return entries;
  }

  // A igual fecha y hora, la que esta mas abajo en el archivo es la mas reciente.
  const newestFirst = (a, b) => (a.key < b.key ? 1 : a.key > b.key ? -1 : b.index - a.index);

  function formatDay(date) {
    const d = new Date(date + "T00:00:00");
    if (isNaN(d)) return date;
    return d.toLocaleDateString("es-ES", { weekday: "long", day: "numeric", month: "long", year: "numeric" });
  }

  let entries = [];

  function render() {
    const q = els.search.value.trim().toLowerCase();
    const day = els.day.value;
    let shown = entries.filter((e) => (day === "all" || e.date === day) && (!q || e.text.includes(q)));
    shown.sort(newestFirst);
    if (els.order.value === "asc") shown.reverse();

    els.note.textContent = shown.length + " de " + entries.length + " entradas";

    if (!shown.length) {
      els.list.innerHTML = '<div class="mem-empty">No hay entradas que coincidan.</div>';
      return;
    }

    let html = "";
    let lastDay = null;
    shown.forEach((e) => {
      if (e.date !== lastDay) {
        html += '<h2 class="mem-day">' + esc(formatDay(e.date)) + "</h2>";
        lastDay = e.date;
      }
      html +=
        '<article class="mem-entry">' +
          '<header class="mem-head">' +
            '<span class="mem-time">' + esc(e.time || "—") + "</span>" +
            "<h3>" + inline(e.title) + "</h3>" +
          "</header>" +
          '<div class="mem-body">' + block(e.lines) + "</div>" +
        "</article>";
    });
    els.list.innerHTML = html;
  }

  function init(md) {
    entries = parse(md);
    const sorted = entries.slice().sort(newestFirst);
    const days = [...new Set(sorted.map((e) => e.date).filter(Boolean))];

    els.total.textContent = entries.length;
    els.days.textContent = days.length;
    if (sorted.length) els.last.textContent = sorted[0].date.slice(5) + (sorted[0].time ? " " + sorted[0].time : "");

    days.forEach((d) => {
      const opt = document.createElement("option");
      opt.value = d;
      opt.textContent = d;
      els.day.appendChild(opt);
    });

    els.search.addEventListener("input", render);
    els.day.addEventListener("change", render);
    els.order.addEventListener("change", render);
    render();
  }

  fetch("memoria.md", { cache: "no-store" })
    .then((r) => { if (!r.ok) throw new Error("HTTP " + r.status); return r.text(); })
    .then(init)
    .catch((err) => {
      els.list.innerHTML =
        '<div class="mem-empty">No se pudo cargar <code>memoria.md</code> (' + esc(String(err.message || err)) +
        '). Abre el panel con <code>web/Iniciar panel.cmd</code> o <code>node web/server.js</code> y entra en ' +
        '<code>http://127.0.0.1:8100/memoria.html</code>.</div>';
    });
})();
