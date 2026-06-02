# Heist Day

A fast-paced, competitive 2v2 multiplayer Third-Person Shooter (TPS) built with Unreal Engine 5. Designed with a strictly authoritative network architecture.

[![GitHub](https://img.shields.io/badge/GitHub-Repository-blue?logo=github)](https://github.com/leikyz/heist-day)
[![Status](https://img.shields.io/badge/Status-Work--In--Progress-orange)](#)
[![Engine](https://img.shields.io/badge/Engine-Unreal%20Engine%205-0E1128?logo=unrealengine)](https://github.com/leikyz/heist-day)

---

## 🚀 Overview

**Heist Day** serves as a core production showcase, focusing heavily on advanced gameplay programming, custom matchmaking integrations, and multiplayer infrastructure optimization. 

Built around a high-stakes round-based system, the project implements strict network synchronization, custom player-swapping lobbies, and tight round management loops. It utilizes **Epic Online Services (EOS)** alongside an external custom-built Go backend to manage lobbies and player orchestration outside the native Unreal Engine scope.

### 🗺️ Core Project Scope
1. **Unreal Engine 5 Framework:** Implements complex network replication, gameplay abilities, and predictive movement.
2. **EOS & External Backend Bridge:** Binds Epic Online Services with a lightweight external Go microservice for robust matchmaking.
3. **Gameplay Architecture:** Structured round management, team assignment, and responsive multi-perspective camera workflows.

---

## ✨ Features

- [x] **Authoritative Multiplayer Foundation:** Core game state, inventory, and health mechanics strictly driven and validated by the server.
- [x] **Go-Powered Matchmaking Bridge:** Interfaces cleanly with an external Go backend to automate lobby pooling and handle initial client coordination.
- [x] **Epic Online Services (EOS) Integration:** Utilizes EOS for identity management, friend sessions, and NAT punch-through.
- [x] **Robust Round Management:** Modular round-based state machine handling warmups, active phases, win/loss evaluation, and post-match cleanups.
- [x] **Advanced Character & Weapon Replication:** Network-optimized projectiles, health bars, and equipment states utilizing precise RPCs and replicated variables.


