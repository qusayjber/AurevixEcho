<div align="center">

# ◉ AUREVIX ECHO

### Your computer remembers how you use it.

**A privacy-first, offline-first desktop application that transforms everyday computer activity into a visual model of how you interact with your machine.**

<br>

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge\&logo=cplusplus\&logoColor=white)](#)
[![CMake](https://img.shields.io/badge/CMake-4.x-064F8C?style=for-the-badge\&logo=cmake\&logoColor=white)](#)
[![SQLite](https://img.shields.io/badge/SQLite-Local%20Storage-003B57?style=for-the-badge\&logo=sqlite\&logoColor=white)](#)
[![Windows](https://img.shields.io/badge/Windows-Native-0078D6?style=for-the-badge\&logo=windows\&logoColor=white)](#)
[![Offline First](https://img.shields.io/badge/Offline--First-Local%20Only-2E7D32?style=for-the-badge)](#privacy)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)](LICENSE)

<br>

**Observe. Discover. Understand.**

</div>

---

## Overview

Aurevix Echo is a native desktop application designed to make computer interaction patterns visible.

Instead of asking you to manually track what you do, Echo observes **non-content computer activity locally** and turns that activity into timelines, workflows, transitions, patterns, replays, and descriptive analytics.

The goal is simple:

> **See how you use your computer without turning your computer into a surveillance system.**

Echo is not a task manager.

It is not an antivirus.

It is not a keylogger.

It is not a productivity judge.

It is not an AI chatbot.

It is a **local behavioral observatory for the relationship between a human and their computer.**

---

## Why Aurevix Echo?

Most software activity trackers focus on productivity scores, reports, or surveillance.

Aurevix Echo takes a different approach.

It focuses on **observable interaction patterns**.

For example:

```text
Chrome
   ↓
IntelliJ IDEA
   ↓
Terminal
   ↓
GitHub
   ↓
IntelliJ IDEA
```

Instead of telling you:

> "You were distracted."

Echo can tell you:

> "You switched applications 42 times."

Instead of assigning a productivity score, it can show:

> "This workflow occurred 17 times."

The application describes behavior rather than judging it.

---

# ✦ Core Features

## 📊 Overview

A high-level view of the computer's activity.

The overview can surface information such as:

* Active time
* Sessions
* Application switches
* Active applications
* Current activity
* Daily workflow

The interface is designed to communicate useful information without turning everything into meaningless statistics.

---

## 🕒 Timeline

A visual representation of application activity over time.

Example:

```text
08:42 ── Chrome          12m
08:54 ── IntelliJ IDEA   37m
09:31 ── Terminal         8m
09:39 ── IntelliJ IDEA   51m
```

The timeline is designed around:

* Day selection
* Hour selection
* Filtering
* Application filtering
* Zooming
* Scrolling
* Application grouping

---

## 🕸 Workflow Graph

One of Echo's signature concepts.

Echo can represent application transitions as a graph:

```text
                 Chrome
                /      \
               /        \
          GitHub       IntelliJ
                          |
                          |
                       Terminal
```

Where:

* **Nodes** represent applications.
* **Edges** represent transitions.
* **Edge frequency** represents repeated transitions.
* **Node size** can represent usage.

The graph is designed to reveal recurring workflows that may be difficult to notice from a normal activity log.

---

## 🧬 Behavior Genome

A descriptive visualization of observed computer interaction patterns.

Example dimensions include:

```text
Context Switching     ██████████░░
Session Depth         ████████████░
Application Diversity ███████░░░░░
Repetition             █████████░░░
Idle Time              ███░░░░░░░░░
Workflow Stability     ████████░░░░
```

### Important

Behavior Genome is **not**:

* A psychological profile
* A personality test
* A mental-health assessment
* An intelligence score
* A productivity score

It describes observed computer interaction patterns only.

---

## 🔎 Pattern Discovery

Echo can discover recurring patterns from actual locally stored activity.

Examples include:

### Repeated Workflow

```text
Chrome → IntelliJ → Terminal
```

### Frequent Context Switching

```text
IntelliJ ↔ Chrome
```

### Long Sessions

```text
IntelliJ IDEA
1h 47m
```

### Repeated Startup Routine

```text
Chrome
   ↓
IntelliJ
   ↓
Terminal
   ↓
GitHub
```

### Time-Based Patterns

```text
This workflow frequently occurs
between 09:00 and 11:00.
```

Patterns are intended to be derived from collected data rather than generated as fictional insights.

Where appropriate, Echo should expose supporting information such as:

* Data range
* Sample size
* Frequency
* Confidence

---

# ▶ Replay

Replay reconstructs a selected period of activity visually.

Example:

```text
08:00 ───────────────────────────── 18:00

       ●
       │
     Chrome
       │
       ●
     IntelliJ
       │
       ●
     Terminal
```

Playback controls can include:

```text
▶ Play
⏸ Pause

1×   2×   5×   10×
```

During replay, the interface can synchronize:

* Applications
* Timeline position
* Graph transitions
* Statistics
* Active sessions

The goal is to make a day of computer activity understandable as a visual sequence.

---

# 💬 What Did I Do?

Echo includes a deterministic local query interface.

It does **not** require an LLM.

Examples:

```text
What did I use most today?

How many times did I switch from Chrome to IntelliJ?

What was my longest session?

What applications did I use before coding?

What workflows repeated this week?
```

Queries are parsed locally using predefined query patterns.

If a query is not supported, Echo should say so instead of pretending to understand it.

---

# ⚡ System Pulse

Echo can also visualize the computer itself.

Possible system metrics include:

* CPU
* RAM
* GPU
* Disk
* Network
* Processes

The intention is not to create another conventional system-monitor dashboard.

Instead, system activity can be correlated with workflow activity.

For example:

```text
IntelliJ session
      +
High CPU activity
      +
Disk activity
```

---

# 🔐 Privacy First

Privacy is a core architectural principle of Aurevix Echo.

The application is designed around:

**Local-only by default.**

**Offline-first.**

**No account required.**

**No cloud database.**

**No API key.**

**No external AI service.**

---

## What Echo Does NOT Collect

Echo must not collect or store:

```text
✕ Keyboard contents
✕ Passwords
✕ Typed text
✕ Chat messages
✕ Clipboard contents
✕ Screenshots
✕ Microphone recordings
✕ Webcam data
✕ Browser page contents
✕ Personal file contents
```

---

## What Echo Can Observe

The application can work with non-content metadata such as:

```text
✓ Application/process name
✓ Application activation time
✓ Duration
✓ Application switching events
✓ Safe file extension/category metadata
✓ Process statistics
✓ CPU usage
✓ RAM usage
✓ Disk activity
✓ Network traffic metadata
✓ Session timestamps
✓ Idle/active periods
```

The objective is to understand **interaction patterns without collecting personal content**.

---

# 🛡 Privacy Center

Echo provides a dedicated privacy area for controlling local monitoring and stored information.

Possible controls include:

* Pause Monitoring
* Stop Monitoring
* Delete History
* Delete All Data
* Export Data
* Data Retention
* Monitoring status
* Startup behavior
* Privacy mode
* Data location

The application should clearly explain what is collected and what is not collected before monitoring begins.

---

# 💾 Local Storage

Aurevix Echo uses SQLite for local storage.

The data model is designed around entities such as:

```text
applications
activity_events
sessions
transitions
system_metrics
daily_summaries
patterns
settings
```

The architecture is designed for:

* Indexed time-based queries
* Batched writes
* Database migrations
* Background processing
* Retention policies
* Graceful recovery from database problems

Local user activity data should not be committed to this repository.

---

# 🏗 Architecture

Aurevix Echo is designed as a native C++ desktop application.

```text
AurevixEcho/
│
├── CMakeLists.txt
├── README.md
├── LICENSE
├── .gitignore
│
├── src/
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── Application
│   │   ├── Configuration
│   │   ├── EventBus
│   │   ├── Logger
│   │   └── TimeManager
│   │
│   ├── monitoring/
│   │   ├── ActivityMonitor
│   │   ├── ProcessMonitor
│   │   ├── WindowMonitor
│   │   ├── SystemMonitor
│   │   └── NetworkMonitor
│   │
│   ├── analytics/
│   │   ├── SessionAnalyzer
│   │   ├── PatternDetector
│   │   ├── WorkflowAnalyzer
│   │   ├── FocusAnalyzer
│   │   └── BehaviorEngine
│   │
│   ├── graph/
│   │   ├── Graph
│   │   ├── GraphNode
│   │   ├── GraphEdge
│   │   ├── GraphAnalyzer
│   │   └── WorkflowGraph
│   │
│   ├── storage/
│   │   ├── Database
│   │   ├── ActivityRepository
│   │   ├── SessionRepository
│   │   └── SettingsRepository
│   │
│   ├── ui/
│   │   ├── Dashboard
│   │   ├── Timeline
│   │   ├── Workflow
│   │   ├── Patterns
│   │   ├── Replay
│   │   ├── Insights
│   │   ├── Privacy
│   │   └── Settings
│   │
│   └── utils/
│
├── resources/
│   ├── icons/
│   └── fonts/
│
└── tests/
```

The actual repository structure may evolve as implementation progresses.

---

# 🧰 Technology

| Technology                 | Purpose                                 |
| -------------------------- | --------------------------------------- |
| **C++20**                  | Core application                        |
| **CMake**                  | Build system                            |
| **SQLite**                 | Local database                          |
| **Windows APIs**           | Native activity/process monitoring      |
| **Dear ImGui / native UI** | Desktop interface                       |
| **Multithreading**         | Background monitoring and database work |

The project is intentionally designed as a native desktop application rather than a web application wrapped inside a desktop shell.

---

# ⚙ Performance

Echo is designed to remain responsive while collecting and analyzing local activity.

Key engineering goals:

* Background monitoring
* Non-blocking UI
* Batched database writes
* Efficient historical queries
* Limited polling
* Aggregated system metrics
* Automatic data retention
* Efficient memory usage
* Graceful handling of large datasets

The UI thread should never freeze because of monitoring or database operations.

---

# 🔒 Security Principles

Aurevix Echo follows several security-oriented principles:

* No sensitive content collection
* No unnecessary network server
* No network ports by default
* Safe filesystem paths
* Input validation
* Path traversal protection
* Graceful database recovery
* No API credentials required
* Local data handling

---

# 🌐 Offline First

The core application is designed to work without an internet connection.

Echo does not require:

```text
✕ OpenAI
✕ Gemini
✕ Claude
✕ Firebase
✕ Supabase
✕ Cloud databases
✕ Paid APIs
✕ API keys
✕ Internet connectivity
```

Required application resources should be bundled locally rather than downloaded at runtime.

---

# 📤 Data Export

Echo can export anonymized local analytics.

Supported formats:

```text
JSON
CSV
```

Example:

```json
{
  "application": "IntelliJ IDEA",
  "duration_seconds": 4210,
  "sessions": 8
}
```

Exports must not contain sensitive content.

---

# 🎨 Design Philosophy

The visual language of Aurevix Echo is built around:

## Calm Intelligence

The interface aims for:

* Spacious layouts
* Rounded surfaces
* Subtle depth
* Soft borders
* Elegant typography
* Controlled gradients
* Minimal visual noise
* High information density without clutter

Supported themes:

```text
Dark
Light
System
```

Animations are intended to be subtle and purposeful rather than flashy.

---

# ♿ Accessibility

The project aims to support:

* Keyboard navigation
* Readable typography
* Appropriate contrast
* Tooltips
* Scalable UI
* Clear focus states
* Reduced-motion mode

---

# 🧪 Development

The project is developed incrementally.

```text
01  C++20 / CMake foundation
        ↓
02  SQLite + settings
        ↓
03  Windows activity monitoring
        ↓
04  Sessions + timeline
        ↓
05  Workflow graph
        ↓
06  Pattern discovery
        ↓
07  Behavior Genome
        ↓
08  Replay
        ↓
09  System Pulse
        ↓
10  Privacy Center + export
        ↓
11  Performance optimization
        ↓
12  UX polish
```

Each stage should leave the application buildable and functional.

---

# 🚀 Building

> Build instructions should be updated to match the exact toolchain and dependencies used by the repository.

### Requirements

* Windows
* C++20-compatible compiler
* CMake
* Required project dependencies
* SQLite
* Git

### Clone

```bash
git clone https://github.com/qusayjber/AurevixEcho.git
cd AurevixEcho
```

### Configure

```bash
cmake -S . -B build
```

### Build

```bash
cmake --build build --config Release
```

### Run

The generated executable location depends on the configured generator and CMake project configuration.

---

# 🧪 Testing

Tests belong under:

```text
tests/
```

The project aims to verify critical components including:

* Database operations
* Session calculation
* Transition calculation
* Pattern detection
* Data retention
* Export
* Settings persistence
* Error handling

---

# 📁 Repository Hygiene

Local activity databases, build artifacts, IDE metadata, executables, logs, and sensitive configuration files should not be committed.

See:

```text
.gitignore
```

for repository exclusions.

---

# 📌 Project Status

> **Development**

Aurevix Echo is under active development.

Features may change as the architecture evolves.

The repository should be considered experimental until a stable release is published.

---

# 🗺 Roadmap

### Foundation

* [x] Project architecture
* [x] C++20 foundation
* [x] CMake integration
* [ ] Automated builds
* [ ] Automated tests

### Monitoring

* [ ] Application detection
* [ ] Process monitoring
* [ ] Window activity
* [ ] System metrics
* [ ] Idle/active detection

### Analytics

* [ ] Session analysis
* [ ] Transition analysis
* [ ] Pattern discovery
* [ ] Workflow analysis
* [ ] Behavior Genome

### Visualization

* [ ] Overview
* [ ] Timeline
* [ ] Workflow Graph
* [ ] Replay
* [ ] Insights
* [ ] System Pulse

### Privacy

* [ ] Privacy Center
* [ ] Monitoring controls
* [ ] Retention controls
* [ ] Data deletion
* [ ] Data export
* [ ] Local data management

### UX

* [ ] First-run onboarding
* [ ] Dark theme
* [ ] Light theme
* [ ] System theme
* [ ] Reduced motion
* [ ] Accessibility improvements

---

# 🤝 Contributing

Contributions, ideas, bug reports, and technical discussions are welcome.

Before contributing:

1. Open an issue describing the proposed change.
2. Keep changes focused.
3. Follow the existing architecture.
4. Avoid introducing unnecessary dependencies.
5. Do not introduce telemetry or external services without discussion.
6. Do not introduce functionality that collects sensitive user content.

---

# 🔐 Privacy & Ethics

Aurevix Echo is designed around a simple principle:

> **Observing behavior does not require collecting content.**

The project intentionally avoids:

* Keylogging
* Clipboard surveillance
* Screen recording
* Microphone recording
* Webcam recording
* Browser content collection
* Personal file-content collection
* Psychological profiling
* Medical claims
* Productivity judgment

Echo should describe what happened rather than tell the user what they should think about it.

---

# 📜 License

Aurevix Echo is released under the **MIT License**.

See [`LICENSE`](LICENSE) for the complete license text.

Copyright © 2026 Qusai Jaber.

---

# 👤 Author

<div align="center">

### Qusai Jaber

**Software Developer · Computer Science · Web & Backend Engineering**

<br>

[![GitHub](https://img.shields.io/badge/GitHub-qusayjber-181717?style=for-the-badge\&logo=github)](https://github.com/qusayjber)

</div>

---

# ✦ Philosophy

Aurevix Echo was built around one question:

> **What if your computer could show you how you use it — without watching what you say or see?**

The result is intended to be more than a monitoring utility.

It is a visual record of interaction between a person and their machine.

```text
Observe.
     ↓
Discover.
     ↓
Understand.
```

---

<div align="center">

### AUREVIX ECHO

**Your computer remembers how you use it.**

Built with C++20 · SQLite · CMake

</div>
