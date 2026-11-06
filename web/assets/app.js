(function () {
  const CW = window.CW;
  if (!CW) return;

  const STATUS_LABEL = { done: "Hecho", doing: "En curso", todo: "Pendiente", blocked: "Bloqueado" };

  const els = {
    brandName: document.getElementById("brandName"),
    brandEngine: document.getElementById("brandEngine"),
    globalPct: document.getElementById("globalPct"),
    globalBar: document.getElementById("globalBar"),
    statDone: document.getElementById("statDone"),
    statDoing: document.getElementById("statDoing"),
    statTodo: document.getElementById("statTodo"),
    statBugs: document.getElementById("statBugs"),
    nowList: document.getElementById("nowList"),
    phases: document.getElementById("phases"),
    search: document.getElementById("search"),
    chips: document.getElementById("chips"),
    metaNote: document.getElementById("metaNote"),
    updated: document.getElementById("updated"),
    source: document.getElementById("source"),
    footCounts: document.getElementById("footCounts")
  };

  const allTasks = [];
  CW.phases.forEach((phase) => {
    phase.tasks.forEach((task) => {
      task.phase = phase.id;
      task.phaseName = phase.name;
      allTasks.push(task);
    });
  });

  const counts = { done: 0, doing: 0, todo: 0 };
  allTasks.forEach((t) => { counts[t.status] = (counts[t.status] || 0) + 1; });
  const total = allTasks.length;
  const pct = total ? Math.round((counts.done / total) * 100) : 0;

  // Header + stats
  document.title = "Hoja de ruta · " + CW.project;
  els.brandName.textContent = CW.project;
  els.brandEngine.textContent = CW.engine;
  els.globalPct.innerHTML = pct + "<span>%</span>";
  els.globalBar.style.width = pct + "%";
  els.statDone.textContent = counts.done;
  els.statDoing.textContent = counts.doing;
  els.statTodo.textContent = counts.todo;
  els.statBugs.textContent = CW.bugs.length;
  els.updated.textContent = CW.updated;
  els.source.textContent = CW.source;
  els.footCounts.textContent = total + " tareas · " + CW.phases.length + " hitos · " + CW.bugs.length + " bugs";

  // "Ahora mismo"
  els.nowList.innerHTML = CW.now.map((item) => {
    const icon = item.status === "done" ? "✔" : item.status === "doing" ? "◐" : "○";
    const cls = item.status === "done" ? "done" : item.status === "doing" ? "doing" : "todo";
    return '<li class="task ' + cls + '" style="border:1px solid var(--line)">' +
      '<span class="tick">' + icon + "</span><span>" + escapeHtml(item.title) + "</span></li>";
  }).join("");

  // Phases
  const openState = {};
  CW.phases.forEach((phase) => {
    const phaseTasks = phase.tasks;
    const done = phaseTasks.filter((t) => t.status === "done").length;
    const doing = phaseTasks.filter((t) => t.status === "doing").length;
    const ppct = phaseTasks.length ? Math.round((done / phaseTasks.length) * 100) : 0;
    const phaseStatus = done === phaseTasks.length ? "done" : (done + doing > 0 ? "doing" : "todo");
    openState[phase.id] = phase.id === CW.currentPhase || phaseStatus === "doing";

    const section = document.createElement("section");
    section.className = "phase " + phaseStatus + (openState[phase.id] ? " open" : "");
    section.dataset.phase = phase.id;

    section.innerHTML =
      '<div class="phase-head">' +
        '<span class="phase-badge">' + escapeHtml(phase.code) + "</span>" +
        "<div>" +
          '<div class="phase-title">' + escapeHtml(phase.name) + "</div>" +
          '<div class="phase-sub">' + escapeHtml(phase.window) + " · " + phaseTasks.length + " tareas</div>" +
        "</div>" +
        '<div class="phase-right">' +
          '<div class="mini-bar"><i style="width:' + ppct + '%"></i></div>' +
          '<span class="count">' + done + "/" + phaseTasks.length + "</span>" +
          '<span class="caret">▶</span>' +
        "</div>" +
      "</div>" +
      '<div class="phase-body">' +
        '<div class="phase-goal"><b>Objetivo:</b> ' + escapeHtml(phase.goal) +
          "<br><b>Puerta de salida:</b> " + escapeHtml(phase.gate) + "</div>" +
        '<div class="tasks"></div>' +
      "</div>";

    const tasksBox = section.querySelector(".tasks");
    phaseTasks.forEach((task) => {
      tasksBox.appendChild(renderTask(task));
    });

    section.querySelector(".phase-head").addEventListener("click", () => {
      section.classList.toggle("open");
      openState[phase.id] = section.classList.contains("open");
    });

    els.phases.appendChild(section);
  });

  function renderTask(task) {
    const row = document.createElement("div");
    row.className = "task " + task.status;
    row.dataset.search = (task.id + " " + task.title + " " + (task.note || "") + " " + (task.evidence || "") + " " + task.phaseName).toLowerCase();

    const icon = task.status === "done" ? "✔" : task.status === "doing" ? "◐" : "";
    let note = "";
    if (task.evidence) note += '<div class="task-note"><b>Evidencia:</b> ' + escapeHtml(task.evidence) + "</div>";
    if (task.note) note += '<div class="task-note">' + escapeHtml(task.note) + "</div>";

    row.innerHTML =
      '<span class="tick">' + icon + "</span>" +
      "<div>" +
        '<div class="task-title">' + (task.status === "done" ? "<s>" + escapeHtml(task.title) + "</s>" : escapeHtml(task.title)) + "</div>" +
        note +
      "</div>" +
      '<span class="task-id">' + escapeHtml(task.id) + "</span>";

    return row;
  }

  // Filters
  let activeFilter = "all";
  const phaseEls = Array.from(document.querySelectorAll(".phase"));

  function applyFilters() {
    const q = els.search.value.trim().toLowerCase();
    let visible = 0;

    phaseEls.forEach((phaseEl) => {
      const rows = Array.from(phaseEl.querySelectorAll(".task"));
      let phaseVisible = 0;

      rows.forEach((row) => {
        const matchText = !q || row.dataset.search.includes(q);
        const matchStatus = activeFilter === "all" || row.classList.contains(activeFilter);
        const show = matchText && matchStatus;
        row.style.display = show ? "" : "none";
        if (show) phaseVisible++;
      });

      const showPhase = phaseVisible > 0;
      phaseEl.style.display = showPhase ? "" : "none";
      if (q || activeFilter !== "all") {
        phaseEl.classList.toggle("open", showPhase);
      } else {
        phaseEl.classList.toggle("open", openState[phaseEl.dataset.phase]);
      }
      visible += phaseVisible;
    });

    els.metaNote.textContent = visible + " de " + total + " tareas visibles";
  }

  els.chips.addEventListener("click", (event) => {
    const btn = event.target.closest(".chip");
    if (!btn) return;
    els.chips.querySelectorAll(".chip").forEach((c) => c.classList.remove("active"));
    btn.classList.add("active");
    activeFilter = btn.dataset.filter;
    applyFilters();
  });

  let debounce;
  els.search.addEventListener("input", () => {
    clearTimeout(debounce);
    debounce = setTimeout(applyFilters, 120);
  });

  els.search.addEventListener("keydown", (e) => {
    if (e.key === "Escape") { els.search.value = ""; applyFilters(); }
  });

  applyFilters();

  function escapeHtml(str) {
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }
})();
