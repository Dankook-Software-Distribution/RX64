# RX64: xv6-riscv → x86_64 포팅

이 저장소는 xv6-riscv(upstream 최신판)를 x86_64 higher-half 커널로 옮기는 프로젝트다.

## Claude가 일하는 방식

- **구현은 사용자가 직접 한다. Claude는 지도만 한다.** 무엇을 할지, 순서, 근거, 함정을 설명하고, 사용자가 쓴 코드를 검토한다. 힌트는 세부 단계(4-a, 4-b …)로 쪼개서 준다. **2026-10-25부터 사용자의 사고력과 문제 해결력을 기르기 위해 힌트는 적당히만 준다**: 목표와 개념, 찾아볼 곳(SDM 절, Linux 파일/함수, OSDev), 스스로 답할 질문, 확인 방법까지. 정확한 레지스터 값·오프셋·완성 코드는 사용자가 요청하거나 시도 후 막혔을 때만 준다. 검토할 때도 고친 코드 대신 버그의 위치와 종류를 짚는다.
- Claude가 직접 고치는 것은 사용자가 요청했을 때뿐이다. 지금까지 요청받은 범위는 주석 작성, 자잘한 수정, 정렬과 순서 정리, Makefile OBJS 추가, 확인용 임시 테스트 코드(`// TEST(단계):` 표시), 커밋 메시지 추천이다. "정석 방법"을 물으면 정석 코드를 보여 줘도 된다.
- 검토할 때는 가능하면 실제로 컴파일하거나 qemu로 부팅해서 값을 확인한다. 매크로 괄호 누락, `>`/`>>` 혼동 같은 조용한 버그를 이렇게 잡았다.
  - 단독 컴파일: `make kernel/foo.o`
  - 부팅: `qemu-system-x86_64 -machine pc -kernel kernel/kernel -m 128M -smp 1 -display none -serial file:<scratch>/ser.txt -no-reboot -d int 2><scratch>/int.txt`
  - monitor: `-monitor stdio`로 `info registers`, `info mem`, `info tlb`, `info pic`, `info lapic`, `x /2gx <addr>`를 파이프로 넣는다(`info tlb`는 XD/PCD/PWT를 `X`/`C`/`T`로 보여 줌. `info irq`는 선이 올라간 횟수라 마스크 확인용이 아님. monitor `x`로 LAPIC MMIO를 읽으면 0이 나옴)
  - 사용자에게 알려 줄 때: `make qemu` 후 `Ctrl-a c`로 monitor, `Ctrl-a x`로 종료
  - 아직 없는 심볼 때문에 링크가 안 될 때는 scratch에 `ld ... --defsym=sym=0`로 임시 링크해서 확인한다
- 사용자와는 한국어로 대화한다.

## 코드 규칙

- riscv판과 달라진 곳에는 `// x86_64:` 태그 주석을 단다. 무엇이 왜 바뀌었는지 riscv판과 대비해서 쓴다. 주석은 xv6 스타일대로 영어로 쓴다.
- 들여쓰기는 `.editorconfig`를 따른다. `.c`/`.h`는 공백 2칸, `.S`는 8칸, Makefile은 탭.
- riscv 원본 파일 전체에 clang-format을 돌리지 않는다. `git diff riscv`로 비교할 때 포팅과 무관한 diff가 섞이기 때문이다.
- 저장소 루트에 Intel SDM PDF(`325462-093-sdm-vol-1-2abcd-3abcd-4.pdf`, 25MB)가 있을 수 있다. 커밋하지 않는다. 페이징 엔트리 형식은 Vol. 3A Figure 5-11(PDF 3292쪽), IDT 게이트는 Figure 7-8.
- 커밋은 파일을 지정해서 한다(`git add <file>`). CLAUDE.md도 커밋한다(2026-10-30 사용자 결정. 이전에는 로컬 전용이었다).
- **커밋 메시지에 `Co-Authored-By: Claude` 같은 Claude 표기를 넣지 않는다**(사용자 지시). 작성자는 사용자뿐이다. 메시지는 핵심만 짧게 쓴다(2026-10-30 사용자 지시): `파일: 내용` 제목 + 무엇을/왜 2~5줄. "Checked in qemu" 같은 확인 결과는 넣지 않는다. 사소한 변경은 제목 + 한 줄.
- 어셈블리(`.S`) 스타일은 xv6를 따른다: 라벨과 `.globl`은 0열, 명령어와 지시어는 공백 8칸, 니모닉과 피연산자 사이는 공백 한 칸, 쉼표 뒤 한 칸. 줄 끝 주석의 열 맞춤은 표처럼 생긴 데이터(헤더, 페이지테이블)에만 한다.
- asm에서 상수를 값으로 쓸 때는 반드시 `$`를 붙인다(`$COM1`, `$V2P_WO(edata)`). 빠뜨리면 메모리 주소로 해석된다. 페이징 전 32비트 코드에서 심볼은 전부 `V2P_WO()`로 감싼다(상위 VA는 32비트 즉시값에 안 들어가 `relocation truncated` 링크 에러가 난다).
- x86.h 헬퍼 이름은 Linux를 따른다: `read_cr2()`, `write_cr3()`, `lgdt()`, `lidt()`, `outb()`. 장치 레지스터 이름과 값도 Linux 헤더를 따르고 필요한 것만 가져온다(`PIC_MASTER_IMR`, `APIC_SPIV` 등, `asm/i8259.h`, `asm/apicdef.h`). 파일 이름은 Linux(`idt.c`, `i8259.c`, `lapic.c`), 초기화 함수는 `xxxinit()`.
- printk 형식은 riscv printf 그대로: `%d %u %x`(32비트), `%ld %lu %lx`(64비트), `%p %c %s %%`. 폭/0 채우기/`%#x`는 없다. `%x`는 `0x`를 붙이지 않으므로 `0x%x`로 쓴다.

## 브랜치

```
riscv          원본. 손대지 않음 (비교 기준)
 └─ x86_64     실제 작업 브랜치 (사용자가 여기서 바로 커밋·푸시한다)
     └─ x86/core     17b2791에서 멈춤. 더 이상 쓰지 않음
```

참고 구현: 커밋 `020fc6a`는 MIT가 2018년에 만든 x86-64 xv6(xv6-public 구조)다.
`git worktree add ../xv6-x86-2018 020fc6a`로 펼쳐서 참고만 한다. 사용자는 MIT 파일을 통째로 합치지 않는다.

**설계 원칙 (2026-10-08 사용자 지시): riscv판 구조를 유지하지 않는다. x86_64 OS의 일반적인 구조와 관례(Linux 등 실제 x86_64 커널의 방식)를 따르되, xv6처럼 교육용이므로 꼭 필요한 기능만 넣는다.** 관례는 "어떻게 하느냐"의 기준이고, "무엇을 넣느냐"는 최소한으로 한다. 지금 단계나 xv6 기능에 필요 없는 것(선택적 보안 기능, 최적화, 쓰지 않는 디스크립터 등)은 넣지 않는다. 설계 선택지를 제안할 때 "riscv판과 같으니까"를 근거로 삼지 말고, x86_64 커널들이 실제로 어떻게 하는지를 근거로 든다. riscv판 코드는 무엇이 필요한지 파악하는 출발점으로만 쓴다. **예외 (2026-10-27): 지금은 조금 힘들어도 나중에 하면 훨씬 힘든 것(디버깅이 어려워지는 것, 구조를 다시 바꿔야 하는 것)은 지금 한다.** 예: TSS를 5단계에, gdt/tss를 처음부터 `struct cpu`에.

## 분담

- 2026-10-10부터 사용자가 **A(부트·메모리), B(트랩·인터럽트·프로세스), C(디바이스·유저·테스트)를 모두** 맡는다. 다른 팀원에게 넘길 파일은 없다.

## 확정된 설계

- KERNBASE `0xffffffff80000000`, `-mcmodel=kernel`. 트램폴린은 없다. 유저 페이지테이블은 커널 PML4[256..511]을 복사한다(실제로 쓰이는 것은 511번 하나).
- trapframe은 커널 스택 꼭대기에 둔다. 레이아웃은 **낮은 주소부터 rax, rbx, rcx, rdx, rbp, rsi, rdi, r8…r15, trapno(120), err(128), rip(136), cs(144), rflags(152), rsp(160), ss(168)**, 총 176바이트다. alltraps가 r15부터 push하므로 rax가 오프셋 0이다.
- GDT는 syscall/sysret 규칙에 맞춘 Linux 순서이고 NSEGS는 7이다: 0 null, 1 KCSEG(0x08), 2 KDSEG(0x10), 3 UDSEG(0x18), 4 UCSEG(0x20), 5·6 TSS. Linux의 32비트 호환 슬롯과 MIT의 KCSEG32는 뺐다. 부팅 GDT도 KCSEG를 1번 슬롯에 둔다.
- 트랩 진입점은 하나다: `vectors.S`의 스텁 256개 → `alltraps` → `trap(tf)`의 `switch (tf->trapno)`. Linux가 예외마다 진입점을 나누는 이유(IST, NMI의 paranoid GS 처리, RCU/lockdep 진입 처리)가 우리에게는 없고, Linux의 IRQ 경로(`irq_entries_start`)와 FRED 경로도 같은 모양이다. 사례가 커지면 `exc_page_fault(tf)`처럼 Linux 이름으로 함수만 뗀다. swapgs와 IST는 쓰지 않는다.
- IDT 게이트는 `struct gate_struct`(Linux 이름, 16바이트), 전부 인터럽트 게이트 `attr 0x8E`. 시스템콜 벡터만 유저 모드 단계에서 DPL=3으로 바꾼다.
- 시스템콜은 1차로 `int $64`를 쓴다. 번호는 rax, 인자는 rdi rsi rdx r10 r8 r9.
- 부팅은 qemu `-kernel` + Multiboot(flag bit 16)로 한다. 머신은 `pc`이고 q35는 쓰지 않는다. 디스크는 IDE PIO, 콘솔은 COM1만 쓴다.
- 커널과 유저 모두 `-mno-sse -mno-mmx`로 빌드하고, 커널은 추가로 `-mno-red-zone`, `-fcf-protection=none`, `-fno-asynchronous-unwind-tables`.
- 유저 영역 상한은 `USERTOP = MAXVA - PGSIZE`다. 맨 위 4KB는 guard로, 나중에 sysret을 도입할 때 non-canonical 주소로 복귀하는 문제를 막는다.
- 주소 규칙: **C 포인터는 항상 VA, 하드웨어가 읽는 칸(PTE, CR3)은 항상 PA.** 경계에서만 `V2P`/`P2V`. kalloc은 KERNBASE 영역의 VA를 주고받는다(이름이 `pa`여도). 디바이스 MMIO 주소에는 V2P를 쓰지 않고 `DEV2V`로 매핑한다.
- 콘솔 출력: `early.c`(`earlyputc/earlyputs/earlyputx`, Linux early_printk 격)와 `printk.c`(riscv printf 기반, 지금은 earlyputc로 출력). console.c는 프로세스(sleep/wakeup, copyin/out)가 생긴 뒤 포팅한다.

## 완료

| 커밋 | 내용 |
|---|---|
| `aa38039` | x86.h: riscv.h 대체. `FL_IF`, `intr_on/off/get`, `r_sp`, PTE 비트(`PTE_P/RW/US/PWT/PCD/A/XD`), `PGMASK`, `PA2PTE`/`PTE2PA`(시프트 없음), `PTE_FLAGS`(XD 보존), `PX` 4레벨, `MAXVA = 1<<47` |
| `6508b1e` | memlayout.h: `EXTMEM`, `PHYSTOP`(PA 128MB), `IOAPIC_PA`, `LAPIC_PA`, `COM1`, `IDE_BASE`, `KERNBASE`, `KERNLINK`, `V2P`/`P2V`(C), `V2P_WO`/`P2V_WO`(asm), `DEVBASE`/`DEV2V`, `KSTACKBASE`/`KSTACK(p)`, `USERTOP` |
| `2a10c73` | Multiboot 스텁, kernel.ld(VMA `KERNLINK`/LMA `EXTMEM`, `etext`/`edata`/`end`), proc.h의 context·trapframe `_Static_assert`, Makefile 골격 |
| `cc2b3a6` | 1a~1c-2: COM1 `'A'`, bss 초기화, `bootstack`, 정적 부팅 페이지테이블(PA 0~1GB를 VA 0과 KERNBASE에 2MB 페이지로) |
| `a25336e` | 1c-3~4: 부팅 GDT, 롱 모드 진입(lgdt → PAE → CR3 → EFER → PG → ljmp), `start64`에서 `'B'` |
| `271fd59` | 1d: `movabs $high64` + `jmp *%rax`로 상위 주소, `'C'`, rsp = 상위 `bootstacktop`, `call main` |
| `65dc441` | x86.h `outb()`, main에서 COM1 문자열 |
| `e2cdb92`, `7b0a37d` | GDT를 Linux 순서로(KCSEG 0x08), gdt.c `seginit()` = `lgdt(gdt)`. CS는 다시 싣지 않음 |
| `17b2791` | traps.h(`T_DE`…`T_CP`), vectors.S(`.rept`/`.altmacro`로 스텁 256개 + `vectors[]`) |
| `4319f48` | riscv 전용 파일 삭제(trampoline.S, kernelvec.S, start.c, plic.c, virtio*). riscv.h는 "아직 포팅 안 된 파일" 표시로 남김 |
| `e4b6707` | early.c, trapasm.S(`alltraps`/`trapret`, iretq) |
| `ae7a643` | idt.c `idtinit()`(게이트 256개 + `lidt`), x86.h `lidt()` |
| `fdbbc1a` | trap.c: trapno/err/rip(#PF면 cr2)를 출력하고 멈춤. x86.h `read_cr2()` |
| `8cc5133` | printk.c, spinlock.c(push_off = pushfq 후 cli), 최소 proc.c(`cpus[]`, `cpuid()`=0, `mycpu()`) |
| `e367a78` | kalloc.c: `freerange(end, P2V(PHYSTOP))`. 부팅 테이블이 덮고 있어 kvminit 전에도 동작 |
| `24663ac`, `d7b8f96`, `796f214` | 3단계 완료. vm.c: walk 4레벨(P2V/V2P, 중간 `P\|RW\|US`, canonical 검사), `kvmmake`는 text `[KERNBASE, etext)` 권한 0 + data/RAM `[etext, P2V(PHYSTOP))` `RW\|XD`, `kvminithart` = `write_cr3(V2P(kernel_pagetable))`. `perm`은 uint64(XD 보존). x86.h `write_cr3()`, `CANONICAL_ADDRESS`/`IS_CANONICAL_ADDRESS`, types.h `int64`. 유저 쪽 vm.c는 `#if 0` |
| `22b9b6b` | 4-a: kvmmake에 `DEV2V(IOAPIC_PA)`, `DEV2V(LAPIC_PA)` 한 페이지씩, `PTE_RW\|PTE_XD\|PTE_PCD\|PTE_PWT`. `info tlb`에서 `X----CT-W` 확인 |
| `da560a8` | 4-b: i8259.c `picinit()` = `PIC_MASTER_IMR`/`PIC_SLAVE_IMR`에 `0xFF`. `info pic`에서 두 칩 `imr=ff`, pic0 `irr=11`(IRQ 0·4 대기), `isr=00`, `irq_base=08` 확인 |
| `8181415` | 4-c: lapic.c. `lapic_read`/`lapic_write`(`*(volatile uint32 *)(DEV2V(LAPIC_PA) + off)`, 바이트 오프셋, static), `lapicinit()` = SPIV(enable \| `T_SPURIOUS`) → TDCR(`APIC_TDR_DIV_1` = 0xB) → LVTT(periodic \| 32) → TMICT(10000000) → LVT0/1 masked → TPR 0, `lapiceoi()`. 끝의 `lapic_read(APIC_LVTT);`는 미사용 에러 회피용으로 사용자가 남김. traps.h에 `T_IRQ0 32`, `IRQ_TIMER 0`, `IRQ_COM1 4`, `IRQ_IDE 14`, `T_SPURIOUS 0xFF`. 확인: LVTT `0x10000` → `0x20020`, `info lapic`에서 enabled, divide 1, current_count 감소, IRR에 32 대기(IF=0이라) |
| `402a120` | 4-d: trap.c `switch (tf->trapno)`. `T_IRQ0 + IRQ_TIMER` → `timerintr()`(`ticks++`, 100번마다 printk, `lapiceoi()`), 나머지는 `show_regs(tf)`(Linux 이름, earlyputs로 trapno/err/cr2/rip/cs/rsp/rflags) 후 멈춤. `uint ticks`는 trap.c에 정의(tickslock은 아직 없음). main 끝에 `intr_on()`(`TEST(4-d)`, 스케줄러가 맡을 때까지) |
| `738e027` | 4-e: ioapic.c. `IO_APIC_REGSEL`(+0x00)/`IO_APIC_WIN`(+0x10) 간접 접근, `ioapicinit()`은 VER bit 16~23로 엔트리 수를 읽어 핀 n → 벡터 `T_IRQ0+n` masked, `ioapicenable(irq, cpu)`는 상위 절반(목적지 `cpu << 24`) 먼저 쓰고 하위 절반으로 unmask. cpu를 LAPIC ID로 씀(-smp 1에서 성립, SMP 때 재검토). claim/complete 없음, EOI는 LAPIC으로 |

현재 Makefile OBJS: entry, main, gdt, early, idt, vectors, trapasm, trap, string, printk, spinlock, proc, kalloc, vm, i8259, lapic, ioapic.
현재 main: `seginit` → `idtinit` → `printkinit` → `kinit` → `kvminit` → `kvminithart` → `picinit` → `lapicinit` → `ioapicinit` → `intr_on()`(TEST) → `for (;;)`.
2026-10-27 확인: 부팅 후 `ABC` 다음 `100`, `200`, `300` …이 찍힌다. 약 4초에 `-d int`의 `v=20`이 381회(≈100Hz, TMICT 10000000 / divide 1이면 tick ≈ 10ms), 다른 벡터 0건. `info lapic`의 ISR/IRR 비어 있음(EOI 정상), `info pic`의 ioapic0 `ver=0x20`.

알아 둘 점:
- PTE 비트 이름은 `PTE_RW`/`PTE_US`다. riscv 코드의 `PTE_R/W/X/U/V`를 포팅할 때 바꾼다. x86엔 R 비트가 없고(P면 읽기 가능), 실행 금지는 `PTE_XD`.
- CR0.WP가 0이라 커널 쓰기는 R/W 비트를 무시한다. 그래서 커널 text가 read-only라도 커널 자신의 쓰기는 막히지 않는다.
- 아직 x86.h에 없는 것: `inb`, `rdmsr`/`wrmsr`(C), `ltr`, `invlpg`, `hlt`. 필요한 단계에서 추가한다.
- lapic.c `lapicinit()` 끝의 `lapic_read(APIC_LVTT);`는 `lapic_read` 미사용 에러를 피하려고 사용자가 남긴 줄이다. 실제로 읽는 곳이 생기면 지운다.
- 임시 코드: main의 `intr_on()`(`TEST(4-d)`). 5단계 스케줄러가 인터럽트를 켜게 되면 지운다. `timerintr()`의 100번마다 출력도 확인용이다.
- 아직 riscv판 파일들이 `riscv.h`를 참조하므로 OBJS 밖 파일은 컴파일되지 않는다. 의도된 것이고, 포팅하면서 하나씩 고친다.
- 겪은 버그: `orl (EFER_LME | EFER_NXE), %eax`에서 `$`가 빠져 triple fault(`-d int`의 `EFER=0`, 예외 `0xe → 0xd → 0x8`이 단서). idt.c에서 `>>`를 `>`로 써서 주소가 0/1로 들어감(컴파일은 통과). kalloc.c에서 `(uint64)pa >= P2V(...)` 정수-포인터 비교가 `-Werror`에 걸림. 4-a에서 디바이스에 `P2V`를 써서 `KERNBASE + 0xFEC00000`이 64비트를 넘어 `0x7ec00000`(유저 영역)으로 감김 → `DEV2V`. 크기 `0xFFFFFFFF - LAPIC_PA`가 페이지 배수가 아님. lapic.c에서 `(volatile uint32 *)x;`처럼 캐스트 결과를 버림(캐스트는 변수 타입을 바꾸지 않는다). TDCR에 `1`을 씀(인코딩상 divide by 4; divide by 1은 0xB). `static` 함수를 아무도 안 부르면 `-Werror=unused-function`.

## 다음 할 일

원칙:
- 처음에는 `-smp 1`로만 돌린다. SMP는 sh 프롬프트가 뜬 다음에 한다.
- Makefile에는 포팅이 끝난 파일만 넣는다. 단계마다 부팅되는 상태로 커밋한다(중간 WIP 커밋은 OBJS 밖 파일이면 허용).
- qemu는 `-d int,cpu_reset -no-reboot -no-shutdown`으로 돌린다.

| 단계 | 할 일 | 확인 방법 |
|---|---|---|
| 0~3 | 완료 | |
| 4 | 완료(`22b9b6b`~`738e027`) | 타이머 tick ≈100Hz ✓ |
| **5 (다음)** | 커널 스택(`proc_mapstacks`), TSS(`struct cpu` 안, `ltr`, rsp0), `swtch.S`(struct context 기반), `proc.c` 스케줄러(`hlt`), 타이머에서 `yield` | 커널 스레드 두 개가 번갈아 돈다 |
| 6 | `uvmcreate`(상위 절반 복사), `uvm*`, `copyin/out`, `freewalk`(256번 이상은 건너뜀), `walkaddr`(`PTE_US`) | 유저 페이지테이블을 만들고 해제해도 커널이 살아 있다 |
| 7 | initcode(`int $64`), IDT 64번 DPL=3, `forkret` → `trapret`, `syscall.c` argraw, `exec.c`(flags2perm → XD, `tf->rip/rsp`), `usys.pl`, uart.c·console.c·ide.c | `exec("/init")` |
| 8 | sh, usertests, 그다음 SMP(CPU 목록: ACPI MADT 또는 MP 테이블 결정, `entryother.S`, `startothers`) | |

4단계에서 정한 것: mp.c(CPU 목록)는 SMP용이라 8단계로 미뤘다. LAPIC/IOAPIC 주소는 qemu pc의 고정값 `LAPIC_PA`/`IOAPIC_PA`를 쓴다. spurious 벡터 `T_SPURIOUS`는 아직 trap()에서 따로 처리하지 않는다(오면 show_regs 후 멈춤. SDM상 spurious에는 EOI를 보내지 않는다).

5단계 세부 (초안. 힌트 줄 때 사용자와 다시 정한다):

| 세부 | 할 일 | 확인 |
|---|---|---|
| 5-a | 커널 스택: kvmmake에 `proc_mapstacks`(프로세스마다 `KSTACK(p)`에 kalloc 페이지, 아래 guard 페이지는 비워 둠, `PTE_RW\|PTE_XD`). Linux의 VMAP_STACK(guard 있는 vmalloc 스택)과 같은 생각 | `info mem`에 KSTACKBASE 위 스택들 |
| 5-b | TSS: 64비트 TSS 구조체(104바이트, `_Static_assert`; Linux `struct x86_hw_tss`: reserved, sp0~sp2, reserved, ist[7], reserved, io_bitmap_base). **`struct cpu` 안에 gdt와 tss를 둔다**(2026-10-27 사용자 결정: SMP 때 구조를 다시 바꾸지 않으려고). gdt.c의 정적 `gdt[]`를 `struct cpu`로 옮기고 `seginit()`이 `mycpu()`의 GDT 5·6번 슬롯에 16바이트 TSS 디스크립터(type 0x9 available 64-bit TSS, base가 여러 조각, limit = sizeof - 1)를 채운 뒤 `lgdt`, `ltr`. io_bitmap_base는 limit보다 크게(I/O 비트맵 없음). x86.h에 `ltr()` 추가. Linux: `cpu_init()` → `load_TR_desc()`, 전환 때 `update_task_stack()` | `ltr`에서 #GP 없음, monitor `info registers`의 `TR`(selector 0x28, base = `&cpus[0].ts`, limit 0x67) |
| 5-c | `swtch.S`: `swtch(struct context *old, struct context *new)`. rdi/rsi로 받아 callee-saved(rbx rbp r12–r15)와 rsp를 old에 저장, new에서 복원. rip은 반환 주소(스택의 return address)로 처리. proc.h의 context 레이아웃(rip 0, rsp 8, rbx 16 …, 64B)과 맞출 것. Linux `__switch_to_asm`은 스택에 push 후 rsp만 교환하는 방식이라 비교 설명 | 커널 스레드 전환 |
| 5-d | proc.c: `procinit`, `allocproc`(kstack = KSTACK, `context.rip`/`context.rsp` 설정), `scheduler`(`intr_on` + 실행할 것 없으면 `hlt`, 프로세스로 전환하기 전에 `mycpu()->ts.rsp0`를 그 프로세스의 커널 스택 꼭대기로), `sched`, `yield`, `sleep`/`wakeup`. x86.h에 `hlt` 추가. 확인용으로 커널 함수 두 개를 프로세스처럼 돌림(`TEST(5)`) | 두 스레드가 번갈아 출력 |
| 5-e | trap.c: 타이머에서 `yield()`(riscv의 `which_dev == 2` → yield에 해당). `tickslock` + `wakeup(&ticks)` | 선점으로 전환됨 |

TSS를 5단계에 넣은 이유(2026-10-27 결정): TSS는 CPL 3 → 0 전환 때만 쓰이지만, 7단계로 미루면 첫 유저 진입에서 틀렸을 때 출력 없이 triple fault가 나고 의심할 것이 너무 많다. 지금은 30줄 정도이고 `ltr`/`TR`로 바로 확인된다. 사용자 원칙: "필요한 것만"이 기본이지만, 나중에 하면 훨씬 힘든 것은 지금 한다.

## 함정

- cr3 수명: 지금 cr3에 실린 페이지테이블은 해제하지 않는다. 스케줄러가 프로세스에서 빠질 때 커널 테이블로 돌아가야 한다.
- `freewalk`가 상위 절반을 건드리면 커널 페이지테이블이 해제된다.
- PTE와 cr3에는 반드시 `V2P`한 값을 넣고, PTE에서 꺼낸 주소는 `P2V`해서 역참조한다. KSTACK과 디바이스 주소에는 V2P를 쓸 수 없다.
- `walk`에서 `va >= MAXVA` 검사는 커널 주소에서 panic한다. 유저 주소 검사는 walkaddr/copyin 쪽에서 한다.
- 유저 cs/ss에는 `|3`을 붙인다. IDT 64번이 DPL=3이 아니면 첫 시스템콜에서 #GP가 난다.
- 중간 페이지테이블 엔트리에 U 비트가 빠지면 유저 #PF가 난다(CPU는 모든 레벨의 권한을 AND).
- TSS 디스크립터는 16바이트(GDT 두 슬롯)이고 CPU마다 하나씩 필요하다(그래서 GDT도 CPU마다). rsp0는 컨텍스트 스위치마다 갱신한다. `ltr`은 디스크립터를 busy(type 0xB)로 바꾸므로 같은 디스크립터로 두 번 `ltr`하면 #GP.
- 인터럽트마다 LAPIC EOI를 하지 않으면 다음 인터럽트가 오지 않는다.
- 커널 스택은 프로세스마다 4KB 한 페이지(riscv xv6와 같음, Linux는 16KB). 넘치면 guard 페이지에서 #PF가 나지만 IST가 없어 같은 스택에 push하다 #DF → 출력 없이 triple fault. 이유 없는 triple fault는 `-d int`에서 `v=0e` 직후 `v=08`이고 cr2가 어떤 `KSTACK(p)` 바로 아래면 스택 넘침이다. 큰 지역 배열과 깊은 재귀를 피한다.
