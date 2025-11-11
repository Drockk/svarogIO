# Git Branch Naming Convention

## Overview
This document defines the branch naming convention for the svarogIO project. A consistent naming scheme improves project organization, enables automated workflows, and makes the repository history easier to navigate.

---

## Branch Categories

### 1. Main Branches
These branches are permanent and protected:

- **`main`** - Production-ready code, stable releases
  - Protected: requires PR approval
  - CI: Full validation (build, test, benchmarks)
  - Deployments: Tagged releases only

- **`develop`** - Integration branch for ongoing development
  - Protected: requires PR approval
  - CI: Full validation on merge
  - Source for feature branches

---

## 2. Development Branches
These branches are temporary and follow a structured naming pattern:

### Pattern: `<type>/<scope>/<short-description>`

**Components**:
- `type`: Branch category (see below)
- `scope`: Project area or phase (optional but recommended)
- `short-description`: Brief kebab-case description

---

### Branch Types

#### `feature/` - New Features
For implementing new functionality.

**Examples**:
- `feature/phase1/execution-context` - Implement execution_context (Phase 1)
- `feature/phase1/work-queue` - Implement work_queue (Phase 1)
- `feature/phase1/io-context` - Implement io_context (Phase 1)
- `feature/phase1/strand` - Implement strand executor (Phase 1)
- `feature/phase2/socket-api` - Implement socket API (Phase 2)
- `feature/phase2/timer-queue` - Implement timer queue (Phase 2)

**Workflow**:
```bash
git checkout develop
git pull origin develop
git checkout -b feature/phase1/execution-context
# ... implement, commit, push ...
# Create PR: feature/phase1/execution-context → develop
```

---

#### `refactor/` - Code Refactoring
For restructuring existing code without changing functionality.

**Examples**:
- `refactor/work-queue-lockfree` - Replace mutex-based queue with lock-free
- `refactor/extract-timer-service` - Extract timer logic into service
- `refactor/platform-abstraction` - Abstract epoll/kqueue differences

**Workflow**:
```bash
git checkout -b refactor/work-queue-lockfree
# ... refactor, commit, push ...
# Create PR: refactor/work-queue-lockfree → develop
```

---

#### `fix/` - Bug Fixes
For fixing defects in existing code.

**Examples**:
- `fix/strand-deadlock` - Fix deadlock in strand serialization
- `fix/memory-leak-io-context` - Fix memory leak in io_context shutdown
- `fix/race-condition-work-queue` - Fix race in work_queue::stop()

**Workflow**:
```bash
git checkout -b fix/strand-deadlock
# ... fix, commit, push ...
# Create PR: fix/strand-deadlock → develop (or main for hotfixes)
```

---

#### `docs/` - Documentation
For documentation-only changes (no code changes).

**Examples**:
- `docs/benchmark-scenarios` - Add benchmark scenario documentation
- `docs/api-reference` - Generate API reference docs
- `docs/phase1-tasks` - Create Phase 1 task list
- `docs/adr-004-executor-design` - Add ADR for executor design

**Workflow**:
```bash
git checkout -b docs/benchmark-scenarios
# ... write docs, commit, push ...
# Create PR: docs/benchmark-scenarios → develop
```

---

#### `test/` - Test Improvements
For adding or improving tests (without changing production code).

**Examples**:
- `test/strand-edge-cases` - Add edge case tests for strand
- `test/benchmark-suite` - Add comprehensive benchmark suite
- `test/integration-phase1` - Add Phase 1 integration tests

**Workflow**:
```bash
git checkout -b test/strand-edge-cases
# ... add tests, commit, push ...
# Create PR: test/strand-edge-cases → develop
```

---

#### `perf/` - Performance Improvements
For optimizations and performance enhancements.

**Examples**:
- `perf/work-queue-cache-line` - Optimize work_queue for cache locality
- `perf/reduce-strand-overhead` - Reduce strand execution overhead
- `perf/io-context-polling` - Optimize io_context polling strategy

**Workflow**:
```bash
git checkout -b perf/work-queue-cache-line
# ... optimize, benchmark, commit, push ...
# Create PR: perf/work-queue-cache-line → develop
```

---

#### `chore/` - Maintenance Tasks
For build system, CI/CD, dependencies, and other maintenance.

**Examples**:
- `chore/cmake-cleanup` - Clean up CMakeLists.txt
- `chore/update-catch2` - Update Catch2 to v3.5.0
- `chore/ci-nightly-workflow` - Add nightly CI workflow
- `chore/clang-tidy-config` - Configure clang-tidy

**Workflow**:
```bash
git checkout -b chore/ci-nightly-workflow
# ... update CI, commit, push ...
# Create PR: chore/ci-nightly-workflow → develop
```

---

#### `experiment/` - Experimental Work
For exploratory work, spikes, and proof-of-concepts (not intended for merge).

**Examples**:
- `experiment/coroutine-integration` - Experiment with C++20 coroutines
- `experiment/boost-lockfree-vs-custom` - Compare Boost.Lockfree vs custom queue
- `experiment/io-uring-backend` - Prototype io_uring backend

**Note**: These branches may be deleted without merging.

---

### Scope Examples for svarogIO

Common scopes based on project structure:

- `phase1` - Phase 1 implementation (execution_context, work_queue, io_context, strand)
- `phase2` - Phase 2 implementation (sockets, buffers, timers)
- `phase3` - Phase 3 implementation (protocols, utilities)
- `execution` - Execution context and executors
- `network` - Networking components
- `io` - I/O operations
- `task` - Task scheduling
- `core` - Core utilities and infrastructure
- `ci` - CI/CD pipelines
- `build` - Build system (CMake)

---

## Naming Rules

### Do's ✅
- Use lowercase letters
- Use hyphens (`-`) to separate words in description
- Keep descriptions concise (3-5 words)
- Include scope when applicable
- Use descriptive names (avoid `fix/bug` or `feature/new`)

**Good Examples**:
- `feature/phase1/execution-context`
- `fix/strand-deadlock`
- `docs/adr-executor-design`
- `perf/io-context-polling`

### Don'ts ❌
- Don't use underscores or camelCase
- Don't use special characters except `/` and `-`
- Don't use vague names
- Don't use issue numbers alone (use descriptive names)

**Bad Examples**:
- `feature/new_stuff` ❌ (underscores, vague)
- `fix/123` ❌ (number only)
- `myBranch` ❌ (camelCase, no type)
- `feature/Phase1ExecutionContext` ❌ (camelCase, no hyphens)

---

## Recommended Workflow for Phase 1

### Step-by-Step Example

1. **Start from `develop`**:
   ```bash
   git checkout develop
   git pull origin develop
   ```

2. **Create feature branch**:
   ```bash
   git checkout -b feature/phase1/execution-context
   ```

3. **Implement and commit**:
   ```bash
   # Make changes
   git add include/svarog/execution/execution_context.hpp
   git commit -m "Add execution_context interface"
   
   git add source/execution/execution_context.cpp
   git commit -m "Implement service registry"
   
   git add tests/execution/execution_context_tests.cpp
   git commit -m "Add execution_context unit tests"
   ```

4. **Push to remote**:
   ```bash
   git push -u origin feature/phase1/execution-context
   ```

5. **Create Pull Request**:
   - Source: `feature/phase1/execution-context`
   - Target: `develop`
   - Title: "Implement execution_context for Phase 1"
   - Description: Reference task list, add notes

6. **After merge, delete branch**:
   ```bash
   git checkout develop
   git pull origin develop
   git branch -d feature/phase1/execution-context
   git push origin --delete feature/phase1/execution-context
   ```

---

## Phase 1 Branch Suggestions

Based on PHASE_1_TASKS.md, here are recommended branch names:

| Task | Branch Name | Estimated Duration |
|------|-------------|-------------------|
| execution_context implementation | `feature/phase1/execution-context` | 1-2 weeks |
| work_queue implementation | `feature/phase1/work-queue` | 2 weeks |
| io_context implementation | `feature/phase1/io-context` | 2-3 weeks |
| strand implementation | `feature/phase1/strand` | 2 weeks |
| Integration tests | `test/phase1-integration` | 1 week |
| Benchmarks | `test/phase1-benchmarks` | 1 week |
| Documentation | `docs/phase1-api-docs` | 3-4 days |

### Alternative: Granular Branches

For smaller, more focused PRs:

| Task | Branch Name |
|------|-------------|
| execution_context interface | `feature/phase1/execution-context-interface` |
| execution_context service registry | `feature/phase1/execution-context-services` |
| work_queue lock-free queue | `feature/phase1/work-queue-lockfree` |
| work_queue thread pool | `feature/phase1/work-queue-workers` |
| io_context event loop | `feature/phase1/io-context-event-loop` |
| io_context executor | `feature/phase1/io-context-executor` |
| strand serialization | `feature/phase1/strand-serialization` |

---

## Special Cases

### Hotfix Branches
For critical production fixes:

**Pattern**: `hotfix/<version>/<short-description>`

**Examples**:
- `hotfix/v1.0.1/critical-memory-leak`
- `hotfix/v1.1.2/security-vulnerability`

**Workflow**:
```bash
git checkout main
git checkout -b hotfix/v1.0.1/critical-memory-leak
# ... fix, test, commit ...
# Create PR: hotfix/v1.0.1/critical-memory-leak → main
# After merge, also merge to develop
```

### Release Branches
For preparing releases:

**Pattern**: `release/<version>`

**Examples**:
- `release/v1.0.0`
- `release/v1.1.0`

**Workflow**:
```bash
git checkout develop
git checkout -b release/v1.0.0
# ... final testing, version bump, changelog ...
# Create PR: release/v1.0.0 → main
# Tag main with v1.0.0
# Merge back to develop
```

---

## Summary

**Default Pattern**: `<type>/<scope>/<short-description>`

**Common Types**:
- `feature/` - New features
- `fix/` - Bug fixes
- `refactor/` - Code refactoring
- `docs/` - Documentation
- `test/` - Tests
- `perf/` - Performance
- `chore/` - Maintenance

**For Phase 1, start with**:
```bash
git checkout -b feature/phase1/execution-context
```

**Recommended PR workflow**:
1. Branch from `develop`
2. Implement feature
3. Create PR to `develop`
4. After approval and CI passing, merge
5. Delete branch

This convention supports the CI/CD workflows defined in `.github/workflows/build.yml` (PR workflow) and `.github/workflows/nightly.yml` (nightly comprehensive build).
