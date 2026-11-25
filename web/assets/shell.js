// Estructura comun del panel: barra lateral, marca, navegacion, progreso global y tema.
// Cada pagina indica cual es con <body data-page="roadmap|kanban|memoria">.
(function () {
  const CW = window.CW || {};
  const page = document.body.dataset.page || "";

  const ICONS = {
    roadmap: '<path d="M9 4 3 6.5v13.5l6-2.5 6 2.5 6-2.5V4l-6 2.5L9 4Z"/><path d="M9 4v13.5M15 6.5V20"/>',
    kanban: '<rect x="3.5" y="4" width="5" height="15" rx="1.5"/><rect x="10" y="4" width="5" height="10" rx="1.5"/><rect x="16.5" y="4" width="4" height="12" rx="1.5"/>',
    memoria: '<circle cx="12" cy="12" r="8.5"/><path d="M12 7.5V12l3 2"/>',
    sun: '<circle cx="12" cy="12" r="4"/><path d="M12 2.5v2M12 19.5v2M4.6 4.6l1.4 1.4M18 18l1.4 1.4M2.5 12h2M19.5 12h2M4.6 19.4 6 18M18 6l1.4-1.4"/>',
    moon: '<path d="M20 14.5A8 8 0 0 1 9.5 4a8 8 0 1 0 10.5 10.5Z"/>',
    search: '<circle cx="11" cy="11" r="6.5"/><path d="m20 20-4.2-4.2"/>'
  };

  const icon = (name, cls) =>
    '<svg class="ico ' + (cls || "") + '" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' +
    (ICONS[name] || "") + "</svg>";

  window.CWIcon = icon;

  // Progreso global (mismo calculo que la hoja de ruta).
  let total = 0;
  let done = 0;
  (CW.phases || []).forEach((p) => p.tasks.forEach((t) => { total++; if (t.status === "done") done++; }));
  const pct = total ? Math.round((done / total) * 100) : 0;
  const openBugs = (CW.bugs || []).filter((b) => String(b.status || "").toLowerCase().indexOf("resuelt") !== 0).length;

  const links = [
    { key: "roadmap", href: "index.html", label: "Hoja de ruta", badge: pct + "%" },
    { key: "kanban", href: "kanban.html", label: "Kanban", badge: openBugs ? openBugs + " bugs" : "" },
    { key: "memoria", href: "memoria.html", label: "Memoria", badge: "" }
  ];

  const sidebar = document.getElementById("sidebar");
  if (sidebar) {
    sidebar.innerHTML =
      '<a class="sb-brand" href="index.html">' +
        '<span class="sb-logo">CW</span>' +
        '<span class="sb-brand-text"><strong>' + esc(CW.project || "Construction World") + "</strong>" +
        "<small>" + esc(CW.engine || "Unreal Engine 5.8") + "</small></span>" +
      "</a>" +
      '<nav class="sb-nav" aria-label="Secciones">' +
        '<p class="sb-label">Seguimiento</p>' +
        links.map((l) =>
          '<a href="' + l.href + '"' + (l.key === page ? ' class="active" aria-current="page"' : "") + ">" +
            icon(l.key) + "<span>" + l.label + "</span>" +
            (l.badge ? '<em class="sb-badge' + (l.key === "kanban" ? " warn" : "") + '">' + esc(l.badge) + "</em>" : "") +
          "</a>"
        ).join("") +
      "</nav>" +
      '<div class="sb-foot">' +
        '<div class="sb-progress">' +
          '<div class="sb-progress-top"><span>Progreso global</span><b>' + pct + "%</b></div>" +
          '<div class="meter"><i style="width:' + pct + '%"></i></div>' +
          "<small>" + done + " de " + total + " tareas completadas</small>" +
        "</div>" +
        '<div class="sb-meta">' +
          '<span class="sb-dot"></span><span>Actualizado <time>' + esc(CW.updated || "—") + "</time></span>" +
        "</div>" +
        '<button class="sb-theme" type="button" id="themeToggle"></button>' +
      "</div>";
  }

  // Tema claro / oscuro (el valor inicial lo pone el script inline del <head>).
  const toggle = document.getElementById("themeToggle");
  function paintToggle() {
    if (!toggle) return;
    const dark = document.documentElement.dataset.theme !== "light";
    toggle.innerHTML = icon(dark ? "sun" : "moon") + "<span>" + (dark ? "Tema claro" : "Tema oscuro") + "</span>";
    toggle.setAttribute("aria-label", dark ? "Cambiar a tema claro" : "Cambiar a tema oscuro");
  }
  if (toggle) {
    toggle.addEventListener("click", () => {
      const next = document.documentElement.dataset.theme === "light" ? "dark" : "light";
      document.documentElement.dataset.theme = next;
      try { localStorage.setItem("cw-theme", next); } catch (e) { /* almacenamiento no disponible */ }
      paintToggle();
    });
    paintToggle();
  }

  // Icono de lupa en los buscadores.
  document.querySelectorAll(".field-search").forEach((el) => {
    el.insertAdjacentHTML("afterbegin", icon("search"));
  });

  // Pie comun.
  const upd = document.getElementById("updated");
  if (upd) upd.textContent = CW.updated || "";
  const src = document.getElementById("source");
  if (src) src.textContent = CW.source || "";

  function esc(str) {
    return String(str == null ? "" : str)
      .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;").replace(/'/g, "&#39;");
  }
})();
