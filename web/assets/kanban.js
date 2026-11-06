(function () {
  const CW = window.CW;
  if (!CW) return;

  document.title = "Kanban · " + CW.project;
  document.getElementById("brandName").textContent = CW.project;
  document.getElementById("brandEngine").textContent = CW.engine;
  document.getElementById("updated").textContent = CW.updated;
  document.getElementById("source").textContent = CW.source;

  const allTasks = [];
  CW.phases.forEach((phase) => {
    phase.tasks.forEach((task) => {
      allTasks.push(Object.assign({}, task, { phase: phase.id, phaseName: phase.name, phaseCode: phase.code }));
    });
  });

  const columns = [
    { key: "todo", title: "TO DO", cls: "todo" },
    { key: "doing", title: "DOING", cls: "doing" },
    { key: "done", title: "DONE", cls: "done" },
    { key: "bugs", title: "BUGS ENCONTRADOS", cls: "bugs" }
  ];

  const state = { q: "", phase: "all", sort: "order" };

  document.getElementById("kTodo").textContent = allTasks.filter((t) => t.status === "todo").length;
  document.getElementById("kDoing").textContent = allTasks.filter((t) => t.status === "doing").length;
  document.getElementById("kDone").textContent = allTasks.filter((t) => t.status === "done").length;
  document.getElementById("kBug").textContent = CW.bugs.length;

  const phaseSelect = document.getElementById("kPhase");
  CW.phases.forEach((phase) => {
    const opt = document.createElement("option");
    opt.value = phase.id;
    opt.textContent = phase.code + " · " + phase.name;
    phaseSelect.appendChild(opt);
  });

  const board = document.getElementById("board");

  function buildBoard() {
    board.innerHTML = "";
    columns.forEach((col) => {
      const colEl = document.createElement("section");
      colEl.className = "col " + col.cls;

      let items = [];
      if (col.key === "bugs") {
        items = CW.bugs.map((bug) => ({
          id: bug.id,
          title: bug.title,
          note: bug.detail,
          status: "bug",
          severity: bug.severity,
          bugStatus: bug.status,
          phaseName: "Bugs"
        }));
      } else {
        items = allTasks.filter((t) => t.status === col.key);
      }

      if (state.phase !== "all" && col.key !== "bugs") {
        items = items.filter((t) => t.phase === state.phase);
      }

      if (state.q) {
        const q = state.q;
        items = items.filter((t) =>
          (t.id + " " + t.title + " " + (t.note || t.detail || "") + " " + (t.phaseName || "") + " " + (t.severity || "")).toLowerCase().includes(q)
        );
      }

      if (state.sort === "phase" && col.key !== "bugs") {
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
    card.className = "card " + (kind === "bugs" ? "bug" : kind);

    let html = '<span class="card-id">' + esc(item.id) + "</span>" +
               '<div class="card-title">' + esc(item.title) + "</div>";

    if (kind === "bugs") {
      const sev = (item.severity || "bajo").toLowerCase();
      html += '<span class="bug-status">' + esc(item.bugStatus || "") + "</span>" +
              '<span class="sev ' + esc(sev) + '">' + esc(item.severity) + "</span>";
    } else {
      html += '<div class="card-sub">' + esc(item.phaseName) + "</div>";
    }

    if (item.note) {
      html += '<div class="card-note">' + esc(item.note) + "</div>";
    } else if (kind === "bugs" && item.note) {
      html += '<div class="card-note">' + esc(item.note) + "</div>";
    }

    card.innerHTML = html;
    return card;
  }

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
