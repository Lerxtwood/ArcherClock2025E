"use strict";

const select = document.getElementById("firmware-version");
const status = document.getElementById("release-status");
const installer = document.getElementById("installer");
const installButton = document.getElementById("install-button");
const retry = document.getElementById("retry-button");
let releases = [];
let toolsReady = false;

function setLink(id, href) {
  const link = document.getElementById(id);
  link.hidden = !href;
  if (href) link.href = href;
  else link.removeAttribute("href");
}

function updateSelection() {
  const release = releases[Number(select.value)];
  installButton.disabled = !release || !toolsReady;
  installer.hidden = !release;
  if (!release) {
    installer.removeAttribute("manifest");
    return;
  }
  installer.setAttribute("manifest", release.manifest);
  document.getElementById("beta-note").hidden = !release.prerelease;
  setLink("notes-link", release.url);
  setLink("factory-link", release.factory);
  setLink("ota-link", release.ota);
  const date = release.published ? new Date(release.published).toLocaleDateString() : "";
  status.textContent = `${release.tag}${date ? " · Published " + date : ""} · ESP32`;
}

async function loadReleases() {
  select.disabled = true;
  retry.hidden = true;
  releases = [];
  updateSelection();
  status.textContent = "Loading GitHub Releases…";
  try {
    const response = await fetch("release-index.json", { cache: "no-store" });
    if (!response.ok) throw new Error(`Release list unavailable (HTTP ${response.status}).`);
    const index = await response.json();
    if (!Array.isArray(index.releases)) throw new Error("The release list is invalid.");
    releases = index.releases.filter(release => release.manifest && release.factory);
    setLink("releases-link", `https://github.com/${index.repository}/releases`);
    select.replaceChildren();
    if (!releases.length) {
      select.add(new Option("No installable releases yet", ""));
      status.textContent = "No full-install images have been published yet. Check GitHub Releases for updates.";
      return;
    }
    releases.forEach((release, i) => {
      select.add(new Option(`${release.name}${release.prerelease ? " (prerelease)" : i === 0 ? " (latest stable)" : ""}`, String(i)));
    });
    select.value = "0";
    select.disabled = false;
    updateSelection();
  } catch (error) {
    select.replaceChildren(new Option("Releases unavailable", ""));
    status.textContent = `${error.message} Try again in a moment.`;
    retry.hidden = false;
  }
}

select.addEventListener("change", updateSelection);
retry.addEventListener("click", loadReleases);
const toolsStatus = document.getElementById("tools-status");
const toolsTimeout = setTimeout(() => {
  toolsStatus.textContent = "USB installer could not load. Check your connection and reload the page. Firmware downloads are still available.";
  toolsStatus.hidden = false;
}, 15000);
customElements.whenDefined("esp-web-install-button").then(() => {
  clearTimeout(toolsTimeout);
  toolsReady = true;
  toolsStatus.hidden = true;
  updateSelection();
});
loadReleases();
