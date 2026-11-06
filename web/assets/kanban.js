(function () {
  const CW = window.CW;
  if (!CW) return;

  const STATUS_LABEL = { todo: "TO DO", doing: "DOING", done: "DONE", bug: "BUG", fixed: "ARREGLADO" };

  document.title = "Kanban · " + CW.project;
  document.getElementById("brandName").textContent = CW.project;
  document.getElementById("brandEngine").textContent = CW.engine;
  document.getElementById("updated").textContent = CW.updated;
  document.getElementById("source").textContent = CW.source;

  const allTasks = [];
  CW.phases.forEach((phase) => {
    phase.tasks.forEach((task) => {
      allTasks.push(Object.assign({}, task, {
        phase: phase.id,
        phaseName: phase.name,
        phaseCode: phase.code,
        kind: "task"
      }));
    });
  });

  const isBugFixed = (bug) => String(bug.status || "").toLowerCase().indexOf("resuelt") === 0;
  const bugsOpen = CW.bugs.filter((b) => !isBugFixed(b));
  const bugsFixed = CW.bugs.filter((b) => isBugFixed(b));

  const columns = [
    { key: "todo", title: "TO DO", cls: "todo" },
    { key: "doing", title: "DOING", cls: "doing" },
    { key: "done", title: "DONE", cls: "done" },
    { key: "bugs", title: "BUGS ENCONTRADOS", cls: "bugs" },
    { key: "fixed", title: "BUGS ARREGLADOS", cls: "fixed" }
  ];

  const state = { q: "", phase: "all", sort: "order" };

  document.getElementById("kTodo").textContent = allTasks.filter((t) => t.status === "todo").length;
  document.getElementById("kDoing").textContent = allTasks.filter((t) => t.status === "doing").length;
  document.getElementById("kDone").textContent = allTasks.filter((t) => t.status === "done").length;
  document.getElementById("kBug").textContent = bugsOpen.length;
  document.getElementById("kFixed").textContent = bugsFixed.length;

  const phaseSelect = document.getElementById("kPhase");
  CW.phases.forEach((phase) => {
    const opt = document.createElement("option");
    opt.value = phase.id;
    opt.textContent = phase.code + " · " + phase.name;
    phaseSelect.appendChild(opt);
  });

  const board = document.getElementById("board");
  const modal = document.getElementById("modal");
  const modalBody = document.getElementById("modalBody");
  let lastFocused = null;

  function itemsForColumn(key) {
    if (key === "bugs") {
      return bugsOpen.map((bug) => Object.assign({}, bug, { kind: "bug", phaseName: "Bugs encontrados" }));
    }
    if (key === "fixed") {
      return bugsFixed.map((bug) => Object.assign({}, bug, { kind: "fixed", phaseName: "Bugs arreglados" }));
    }
    return allTasks.filter((t) => t.status === key);
  }

  function buildBoard() {
    board.innerHTML = "";

    columns.forEach((col) => {
      const colEl = document.createElement("section");
      colEl.className = "col " + col.cls;

      let items = itemsForColumn(col.key);

      if (state.phase !== "all" && (col.key === "todo" || col.key === "doing" || col.key === "done")) {
        items = items.filter((t) => t.phase === state.phase);
      }

      if (state.q) {
        const q = state.q;
        items = items.filter((t) =>
          (t.id + " " + t.title + " " + (t.note || t.detail || "") + " " + (t.phaseName || "") + " " + (t.severity || "") + " " + logsOf(t).map((l) => l.text).join(" "))
            .toLowerCase()
            .includes(q)
        );
      }

      if (state.sort === "phase" && (col.key === "todo" || col.key === "doing" || col.key === "done")) {
        items.sort((a, b) => (a.phaseName + a.id).localeCompare(b.phaseName + b.id));
      }

      colEl.innerHTML =
        '<div class="col-head">' +
          '<span class="col-dot"></span>' +
          '<span class="col-title">' + col.title + "</span>" +
          '<span class="col-count">' + items.length + "</span>" +
        "</div>" +
        '<div class="col-body"></div>';

      const body = colEl.querySelector(".col-body");
      if (!items.length) {
        body.innerHTML = '<div class="empty" style="padding:18px 8px;font-size:12px">Sin tarjetas</div>';
      } else {
        items.forEach((item) => body.appendChild(renderCard(item, col.key)));
      }

      board.appendChild(colEl);
    });

    const visible = Array.from(board.querySelectorAll(".card")).length;
    document.getElementById("kNote").textContent = visible + " tarjetas visibles";
  }

  function renderCard(item, kind) {
    const card = document.createElement("article");
    const cls = kind === "bugs" ? "bug" : kind === "fixed" ? "fixed" : kind;
    card.className = "card " + cls;
    card.tabIndex = 0;
    card.setAttribute("role", "button");
    card.setAttribute("aria-label", "Abrir detalle de " + item.id + ": " + item.title);

    let html = '<span class="card-id">' + esc(item.id) + '<span class="card-open" aria-hidden="true">Abrir ›</span></span>' +
               '<div class="card-title">' + esc(item.title) + "</div>";

    if (kind === "bugs" || kind === "fixed") {
      const sev = (item.severity || "bajo").toLowerCase();
      html += '<div class="card-foot">' +
                '<span class="sev ' + esc(sev) + '">' + esc(item.severity) + "</span>" +
                '<span class="bug-status">' + esc(item.status || "") + "</span>" +
              "</div>";
    } else {
      html += '<div class="card-sub">' + esc(item.phaseName) + "</div>";
    }

    const note = item.note || item.detail;
    if (note) {
      html += '<div class="card-note">' + esc(note) + "</div>";
    }

    const logs = logsOf(item);
    if (logs.length) {
      html += '<span class="card-log">' + logs.length + (logs.length === 1 ? " anotación" : " anotaciones") + "</span>";
    }

    card.innerHTML = html;

    const open = () => openCard(item, kind);
    card.addEventListener("click", open);
    card.addEventListener("keydown", (event) => {
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        open();
      }
    });

    return card;
  }

  function logsOf(item) {
    return Array.isArray(item.log) ? item.log : [];
  }

  function openCard(item, kind) {
    lastFocused = document.activeElement;

    const logs = logsOf(item);
    const statusLabel = kind === "fixed" ? "ARREGLADO" : STATUS_LABEL[kind] || kind.toUpperCase();
    const note = item.note || item.detail || "";

    let html =
      '<div class="modal-top">' +
        '<span class="modal-id">' + esc(item.id) + "</span>" +
        '<span class="pill ' + (kind === "bugs" ? "blocked" : kind === "fixed" ? "done" : kind) + '">' + esc(statusLabel) + "</span>" +
      "</div>" +
      "<h2 id=\"modalTitle\">" + esc(item.title) + "</h2>";

    const meta = [];
    if (item.phaseName) meta.push(["Hito", item.phaseName]);
    if (item.phaseCode) meta.push(["Código", item.phaseCode]);
    if (item.window) meta.push(["Ventana", item.window]);
    if (item.severity) meta.push(["Severidad", item.severity]);
    if (item.status && (kind === "bugs" || kind === "fixed")) meta.push(["Estado", item.status]);
    if (item.status && kind !== "bugs" && kind !== "fixed") meta.push(["Estado", STATUS_LABEL[item.status] || item.status]);

    if (meta.length) {
      html += '<dl class="modal-meta">' + meta.map((pair) =>
        "<div><dt>" + esc(pair[0]) + "</dt><dd>" + esc(pair[1]) + "</dd></div>"
      ).join("") + "</dl>";
    }

    if (note) {
      html += '<div class="modal-section"><h3>' + (kind === "bugs" || kind === "fixed" ? "Descripción" : "Notas") + "</h3>" +
              '<p class="modal-note">' + esc(note) + "</p></div>";
    }

    if (item.evidence) {
      html += '<div class="modal-section"><h3>Evidencia</h3><p class="modal-note">' + esc(item.evidence) + "</p></div>";
    }

    html += '<div class="modal-section"><h3>Trabajo realizado</h3>';
    if (logs.length) {
      html += '<ol class="timeline">' + logs.map((entry) =>
        '<li><time>' + esc(entry.date) + "</time><p>" + esc(entry.text) + "</p></li>"
      ).join("") + "</ol>";
    } else {
      html += '<p class="modal-empty">Todavía no hay anotaciones en esta tarjeta.</p>';
    }
    html += "</div>";

    modalBody.innerHTML = html;
    modal.hidden = false;
    document.body.classList.add("modal-open");
    modal.querySelector(".modal-card").focus();
  }

  function closeCard() {
    if (modal.hidden) return;
    modal.hidden = true;
    document.body.classList.remove("modal-open");
    if (lastFocused && typeof lastFocused.focus === "function") lastFocused.focus();
  }

  modal.addEventListener("click", (event) => {
    if (event.target.hasAttribute("data-close") || event.target === modal) closeCard();
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape") closeCard();
  });

  let debounce;
  document.getElementById("kSearch").addEventListener("input", (e) => {
    clearTimeout(debounce);
    debounce = setTimeout(() => { state.q = e.target.value.trim().toLowerCase(); buildBoard(); }, 120);
  });
  phaseSelect.addEventListener("change", (e) => { state.phase = e.target.value; buildBoard(); });
  document.getElementById("kSort").addEventListener("change", (e) => { state.sort = e.target.value; buildBoard(); });

  buildBoard();

  function esc(str) {
    return String(str == null ? "" : str)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }
})();
