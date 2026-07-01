# 👩‍💻 Contributing & Open-Source Collaboration Guide

This repository welcomes open-source additions, performance optimization patches, and analytical issue reports. Please follow these guidelines to maintain standard engineering methodologies across code updates.

---

## 📋 Table of Contents
1. [Reporting Technical Issues](#-reporting-technical-issues)
2. [Git Workflow & Branching Strategy](#-git-workflow--branching-strategy)
3. [Commit Message Conventions](#-commit-message-conventions)
4. [Submitting Pull Requests](#-submitting-pull-requests)

---

## 🐛 Reporting Technical Issues

Before opening a new bug report or feature request in the GitHub issue tracker, please verify that the behavior has not already been covered in existing threads. 

When creating a new **Issue**, utilize the following structured layout:
* **Environment Context:** Provide your current Linux Kernel release via `uname -r`, your distribution flavor (e.g. Ubuntu 22.04), and the installed BCC version.
* **Expected Behavior:** A clear description of what should occur under nominal execution paths.
* **Actual Error Footprint:** The exact log outputs, compiler warning strings, or unexpected error traces extracted from `firewall.log`.
* **Minimal Reproducible Example (MRE):** A short code snippet or step-by-step terminal script that reliably reproduces the reported fault.

---

## 🌿 Git Workflow & Branching Strategy

This project uses a standard branching model to protect the integrity of the active production branch:
* **`main` / `master` Branch:** Reflects the stable, production-tested state of the codebase. Direct pushes to this branch are restricted.
* **Feature Branches (`feature/`)**: Dedicated to implementing new capabilities or functional layers (e.g., `feature/cidr-support`).
* **Fix Branches (`bugfix/` / `hotfix/`)**: Reserved for targeted debugging, fixing kernel driver compile warnings, or addressing user-space exceptions.

### Creating a development branch:
<pre><code class="language-bash"># Pull the latest upstream updates
git checkout main
git pull origin main

# Allocate a isolated feature workspace
git checkout -b feature/your-meaningful-feature-name</code></pre>

---

## ✍️ Commit Message Conventions

Commit messages must follow clear formatting rules to keep the project history descriptive and maintainable. We use structural prefixes to categorize changes:

| Prefix Token | Structural Scope Reference | Example Context Definition |
| :--- | :--- | :--- |
| `feat:` | Introducing a completely new capability or function. | `feat: add live configuration export schemas` |
| `fix:` | Correcting an explicit runtime bug, exception, or leak. | `fix: patch memory leak in stats thread` |
| `kernel:` | Modifying the Restricted C data plane code. | `kernel: optimize boundary checks on udp parsing` |
| `docs:` | Editing Markdown documentation, diagrams, or comments. | `docs: add usage guide for python integrations` |

---

## 🚀 Submitting Pull Requests

Follow this engineering sequence to submit your changes via a **Pull Request (PR)**:

1. **Keep PRs Focused:** Ensure your pull request targets a single, well-defined enhancement or fix. Large, multi-topic PRs are harder to review and take longer to merge.
2. **Sync with Upstream Changes:** Rebase or merge the latest changes from the primary upstream `main` branch into your working feature branch to resolve any merge conflicts early:
   <pre><code class="language-bash">git fetch origin
   git merge origin/main</code></pre>
3. **Open the Pull Request:** Submit your request against the upstream repository. In the description, link to any relevant issue trackers (e.g., `Closes #14`) and provide a brief summary of the changes you implemented.
4. **Code Review & Updates:** Maintainers will review your submission and may suggest updates. If modifications are requested, apply them directly to your local branch and push the new commits; the Pull Request will update automatically.