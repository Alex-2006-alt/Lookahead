# Third-Party Assets & Licenses

All third-party libraries, test oracles, and fonts used in Lookahead are recorded here with their respective open licenses. No GPL code is included in the engine.

---

## 1. Engine Core & Native Targets

* **Lookahead Engine Source**:
  * Author: Alex (`Alex-2006-alt`)
  * License: MIT License (see [LICENSE](../LICENSE))
* **GoogleTest**:
  * Purpose: Native unit test harness.
  * License: BSD 3-Clause License.

---

## 2. Testing Tools & Oracles

* **python-chess**:
  * Purpose: External test oracle used exclusively in `tools/` for verifying perft counts, legal move differential tests, and match runner orchestration. **Never included in the engine build.**
  * License: GNU General Public License v3.0 (GPLv3) — isolated exclusively to developer-side test scripts in `tools/`.

---

## 3. Web Application & UI Assets

* **React**:
  * Purpose: User interface framework.
  * License: MIT License.
* **Vite**:
  * Purpose: Frontend build tool and bundler.
  * License: MIT License.
* **Tailwind CSS**:
  * Purpose: Utility-first CSS styling framework.
  * License: MIT License.
* **Typography**:
  * *Instrument Serif*: SIL Open Font License 1.1.
  * *Inter*: SIL Open Font License 1.1.
  * *JetBrains Mono*: Apache 2.0 License.
* **Chess Piece Glyphs**:
  * Purpose: Scalable SVG piece rendering.
  * Attribution: Standard Open Source Cburnett / Wikimedia Commons chess vectors (CC BY-SA 3.0 / Public Domain).
