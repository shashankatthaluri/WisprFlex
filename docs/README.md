WisprFlex — Documentation Control Center

This is the ONLY required entry point for all agents and contributors.
If you are unsure what to do, start here.

1. What is WisprFlex?

WisprFlex is a cross-platform, offline-first voice-to-text dictation and transcription app designed for professionals who value:

Speed

Low resource usage

Privacy

Minimal UI friction

The project is being built using agent-based execution, which requires strict rules, clarity, and discipline.

2. Current Project Phase

🚦 Current Phase: Phase 0 — Audit & Decomposition

Phase 0 Rule:
❌ No code changes
❌ No refactors
❌ No dependency updates
✅ Read, measure, document only

If you are not working on Phase 0 tasks, you are working on the wrong thing.

3. Mandatory Reading Order (DO NOT SKIP)

All agents must read in this order:

00-overview.md

01-architecture.md

02-tech-decisions.md

03-product-scope-v1.md

04-agent-workflow.md

05-roadmap.md

Skipping steps leads to invalid output.

4. Role-Based Rules (Very Important)

After reading the above, read only your role file:

Your Role	Read This
Product Owner	/docs/roles/product-owner.md
Tech Lead	/docs/roles/tech-lead.md
Designer	/docs/roles/designer.md
Core Engineer	/docs/roles/core-engineer.md
App Engineer	/docs/roles/app-engineer.md
QA	/docs/roles/qa.md

You must not act outside your role authority.

5. Phase 0 — Required Outputs

All Phase 0 work must result in exactly these files:

/docs/phase-0/
 ├── SYSTEM_MAP.md
 ├── DATA_FLOW.md
 ├── DEPENDENCY_AUDIT.md
 ├── PERFORMANCE_BASELINE.md
 ├── KEEP_DROP_REWRITE.md
 └── RISKS_AND_CONSTRAINTS.md


If a file is missing → Phase 0 is incomplete.

6. Rules of Authority

Product Owner

Owns scope and priorities

Tech Lead

Owns architecture and performance

Agents

Execute tasks exactly as specified

QA

Can block progression to next phase

No agent may:

Change architecture

Add features

Modify scope

Change licenses

Without explicit approval.

7. If You Are Confused

Do not guess.

Follow this order:

Re-read this README

Re-read your role doc

Check the current phase

Escalate with a written question

Silent assumptions are treated as failures.

8. How We Proceed From Here
Immediate Next Step

➡️ Execute Phase 0 agent tasks exactly as specified

When Phase 0 outputs are complete:

Product Owner + Tech Lead review

Explicit “Phase 0 Complete” decision

Only then do we move to Phase 1

9. Final Rule

Clarity beats speed. Discipline beats talent.

This README governs all documentation.
If anything conflicts — this file wins.