# RX64: xv6-riscv → x86_64 포팅

이 저장소는 xv6-riscv(upstream 최신판)를 x86_64 higher-half 커널로 옮기는 프로젝트다.

## Claude가 일하는 방식

- **x86 고유 부분은 사용자가 직접 구현하고, Claude는 지도한다.** 무엇을 할지, 순서, 근거, 함정을 설명하고, 사용자가 쓴 코드를 검토한다. 힌트는 세부 단계(4-a, 4-b …)로 쪼개서 준다.
- **힌트는 구체적으로 준다(2026-10-30 사용자 결정. 2026-10-25의 "적당히만" 방침을 대체한다)**: 목표, 개념, 찾아볼 곳에 더해 정확한 값·비트 위치·오프셋·필드 배치·호출 순서·함정까지 준다. 완성 코드는 주지 않는다. 사용자가 요청하면 준다. 검토할 때는 버그의 위치와 종류, 고치는 방향을 구체적으로 짚는다.
- **기계적 포팅은 Claude가 한다(2026-10-30 사용자 결정)**: 아키텍처와 거의 무관한 파일(`bio`, `log`, `fs`, `file`, `pipe`, `sleeplock`, `sysfile` 등)의 `riscv.h` → `x86.h` 교체, 이름 변경, 자잘한 타입 수정, Makefile OBJS 추가. 바뀐 곳에는 규칙대로 `// x86_64:` 주석을 달고, 사용자가 검토한다. x86 고유 부분(스케줄러, `sp0`, 유저 진입, 시스템콜 인자, `exec`, `uart`/`ide`, SMP)은 사용자가 한다.
- 그 밖에 Claude가 직접 고치는 것은 사용자가 요청했을 때뿐이다. 지금까지 요청받은 범위는 주석 작성, 자잘한 수정, 정렬과 순서 정리, 확인용 임시 테스트 코드(`// TEST(단계):` 표시), 커밋 메시지 추천, CLAUDE.md 갱신이다. "정석 방법"을 물으면 정석 코드를 보여 줘도 된다.
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
- 저장소 루트에 Intel SDM PDF(`325462-093-sdm-vol-1-2abcd-3abcd-4.pdf`, 25MB)가 있을 수 있다. 커밋하지 않는다. 페이징 엔트리 형식은 Vol. 3A Figure 5-11(PDF 3292쪽), IDT 게이트는 Figure 7-8, TSS 디스크립터는 Figure 10-4(약 3449쪽), 64비트 TSS는 Figure 10-11(약 3462쪽).
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
- GDT는 syscall/sysret 규칙에 맞춘 Linux 순서이고 NSEGS는 7이다: 0 null, 1 KCSEG(0x08), 2 KDSEG(0x10), 3 UDSEG(0x18), 4 UCSEG(0x20), 5·6 TSS. Linux의 32비트 호환 슬롯과 MIT의 KCSEG32는 뺐다. 부팅 GDT도 KCSEG를 1번 슬롯에 둔다. GDT와 TSS는 CPU마다 `struct cpu`의 `gdt[NSEGS]`, `tss`(`struct x86_hw_tss`)에 있다. `seginit()`이 상수 `gdt_template`을 복사하고, `set_tss_desc()`로 5·6번(`TSSSEG` 0x28)을 채운 뒤 `lgdt`, `ltr`을 한 번 한다(Linux처럼 전환 때는 `tss.sp0`만 바꾼다. MIT판처럼 매번 `ltr`하지 않는다).
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
| `069f112` | 5-a: proc.c `proc[]`, `proc_mapstacks()`(`char *stack = kalloc()`, `V2P(stack)`, `PTE_RW\|PTE_XD`), kvmmake 끝에서 호출. `info mem`에 4KB 스택 64개(`0xffffffff88001000`~`0xffffffff88080000`, 사이사이 guard) |
| `830844c` | 5-c: swtch.S. callee-saved와 rsp를 old에 저장, 반환 주소는 `popq`로 `old->rip`, new는 `jmp *0(%rsi)`로 재개(ret 아님) |
| `689ffd7` | proc.c `procinit()`(`p->kstack = KSTACK`), `allocpid`, `find_unused()`(p->lock 잡고 반환), `allocproc()`(trapframe = 스택 꼭대기 − 176, `context.rsp` = trapframe − 8로 진입 시 `rsp % 16 == 8`, `context.rip` = `forkret`), `forkret`은 panic 스텁. main에 `procinit()`, `allocproc_test()`(`TEST(5-c-1)`) |
| `014bec5` | 5-b: x86.h `struct x86_hw_tss`(packed, 크기 104·`sp0` 오프셋 4 assert), `TSSSEG`, `ltr()`. proc.h `struct cpu`에 `gdt`, `tss`. gdt.c `gdt_template`, `set_tss_desc()`, `seginit()` = 복사 → TSS 디스크립터 → `lgdt` → `ltr`. 확인: `TR =0028`, base `&cpus[0].tss`, limit 0x67, GDT 메모리의 디스크립터가 `...8b...`(busy) |
| `65c6046` | ioapic.c `crst` → `maxredir`, CLAUDE.md 커밋 시작 |
| `7936650` | 5-d·5-e: proc.c `myproc`, `scheduler`(루프 위 `intr_off()`만, `swtch` 직전 `c->tss.sp0 = p->kstack + PGSIZE`, 할 일 없으면 `safe_halt()`), `sched`/`yield`/`sleep_prepare`/`sleep`/`wakeup`(riscv 그대로), `forkret`(p->lock 해제 후 `ret`으로 8바이트 칸에 복귀), 함수 순서를 riscv판에 맞춤(`extern void forkret(void)` 전방 선언). x86.h `safe_halt()`(`sti; hlt`). trap.c `tickslock`, `trapinit()`, `timerintr()`는 CPU 0만 ticks++·`wakeup(&ticks)`, EOI는 모든 CPU, `trap()` 끝에서 타이머이고 `myproc() != 0`이면 `yield()`. main은 `kthread_test()`(TEST(5)) 후 `scheduler()` |
| (이 커밋) | 6단계: vm.c 유저 쪽 `#if 0` 해제. 6-a(Claude) `PTE_V/W/U` → `PTE_P/RW/US`, `PTE_R` 삭제. `uvmcreate`는 `kernel_pagetable[256..511]`을 값 그대로 복사(하위 테이블 공유). `freewalk(pagetable, level)`(PML4 = 3, PML4에서는 0~255만, `level > 0`이면 가지, level 0에서 present면 leaf panic, 자식은 `P2V`), `uvmfree`는 3으로 호출. PTE에 넣는 곳은 `V2P(mem)`, 읽는 곳은 `P2V(pa)`. `vmfault`는 `PTE_RW\|PTE_US\|PTE_XD`로 매핑하고 PA 반환(walkaddr와 같게). `uvmalloc` xperm·`uvmcopy` flags는 uint64. `walkaddr`/`copyout` 상한은 `USERTOP`. TEST(6): vm.c `uvm_test()`, kalloc.c `kfreecount()`. Makefile `-D qemu.log` |

현재 Makefile OBJS: entry, main, gdt, early, idt, vectors, trapasm, trap, string, printk, spinlock, proc, kalloc, vm, i8259, lapic, ioapic, swtch.
현재 main: `seginit` → `idtinit` → `printkinit` → `kinit` → `kvminit` → `kvminithart` → `procinit` → `trapinit` → `picinit` → `lapicinit` → `ioapicinit` → `uvm_test()`(TEST(6)) → `kthread_test()`(TEST(5)) → `scheduler()`.
2026-10-30 확인: 부팅 후 `ABC` 다음 `a`/`b`가 선점으로 섞여 찍힌다(`aa`, `bb`가 가끔 보임). `-d int`에 `v=20`만 약 95Hz. scratch 사본에서 두 스레드가 `sys_sleep` 패턴으로 50 tick씩 잠들게 해 hlt 경로(`myproc() == 0`일 때 타이머)와 `wakeup(&ticks)`도 확인했다.
2026-10-30 확인(6단계): `uvm_test: ok`. uvmalloc 2MB+3페이지(PT 두 개), `USERTOP - PGSIZE`(PML4 255번) 매핑, walkaddr guard, copyout/copyin/copyinstr(페이지 경계), uvmcopy, 유저 cr3로 전환해도 printk 동작, 해제 후 빈 페이지 수 동일. scratch에서 freewalk 512·uvmcopy VA·uvmcreate 빈 테이블로 망가뜨리면 각각 leaf panic, #GP, triple fault로 잡힌다.

알아 둘 점:
- PTE 비트 이름은 `PTE_RW`/`PTE_US`다. riscv 코드의 `PTE_R/W/X/U/V`를 포팅할 때 바꾼다. x86엔 R 비트가 없고(P면 읽기 가능), 실행 금지는 `PTE_XD`.
- CR0.WP가 0이라 커널 쓰기는 R/W 비트를 무시한다. 그래서 커널 text가 read-only라도 커널 자신의 쓰기는 막히지 않는다.
- 아직 x86.h에 없는 것: `inb`, `rdmsr`/`wrmsr`(C), `invlpg`. 필요한 단계에서 추가한다. 대기는 `safe_halt()`(`sti; hlt`)를 쓴다.
- lapic.c `lapicinit()` 끝의 `lapic_read(APIC_LVTT);`는 `lapic_read` 미사용 에러를 피하려고 사용자가 남긴 줄이다. 실제로 읽는 곳이 생기면 지운다.
- 임시 코드: proc.c `test_thread`/`test_a`/`test_b`/`kthread_test()`와 main의 호출, defs.h 선언(`TEST(5)`). `allocproc()`으로 만든 프로세스의 8바이트 칸(`*(uint64 *)p->context.rsp`)에 함수 주소를 넣어 `forkret`의 `ret`이 그리로 가게 한다. 스레드는 `intr_on()`으로 시작하고 return하면 안 된다(위가 trapframe). 7단계 `userinit`이 들어오면 지운다. `timerintr()`의 100 tick 출력은 사용자가 뺐다. `TEST(6)`: vm.c 끝의 `uvm_test()`/`test6_check()`, kalloc.c `kfreecount()`, defs.h 선언, main 호출. 7단계에서 같이 지운다.
- `forkret`은 p->lock을 풀고 `ret`으로 `allocproc`이 남긴 8바이트 칸에 돌아간다. 7단계에서 그 칸에 `trapret` 주소를 넣는다. 이 경로로 들어간 함수는 진입 시 `rsp % 16 == 0`이다(`-mno-sse`라 문제없음). riscv판의 `fsinit`/`kexec`(첫 프로세스)는 7단계에 넣는다.
- defs.h의 `trapinithart` 선언은 쓰이지 않는다(그 역할은 `idtinit()`). 정리할 때 지운다.
- 아직 riscv판 파일들이 `riscv.h`를 참조하므로 OBJS 밖 파일은 컴파일되지 않는다. 의도된 것이고, 포팅하면서 하나씩 고친다.
- 겪은 버그: `orl (EFER_LME | EFER_NXE), %eax`에서 `$`가 빠져 triple fault(`-d int`의 `EFER=0`, 예외 `0xe → 0xd → 0x8`이 단서). idt.c에서 `>>`를 `>`로 써서 주소가 0/1로 들어감(컴파일은 통과). kalloc.c에서 `(uint64)pa >= P2V(...)` 정수-포인터 비교가 `-Werror`에 걸림. 4-a에서 디바이스에 `P2V`를 써서 `KERNBASE + 0xFEC00000`이 64비트를 넘어 `0x7ec00000`(유저 영역)으로 감김 → `DEV2V`. 크기 `0xFFFFFFFF - LAPIC_PA`가 페이지 배수가 아님. lapic.c에서 `(volatile uint32 *)x;`처럼 캐스트 결과를 버림(캐스트는 변수 타입을 바꾸지 않는다). TDCR에 `1`을 씀(인코딩상 divide by 4; divide by 1은 0xB). `static` 함수를 아무도 안 부르면 `-Werror=unused-function`. 5단계: `scheduler()`의 바깥 `for (;;)`를 빠뜨림(`noreturn` 선언 덕에 `-Werror`로 잡힘). `lapiceoi()`를 `if (cpuid() == 0)` 안에 넣음(-smp 1에선 증상 없음, SMP에서 다른 CPU의 타이머가 멈춤). `trap()`의 `yield()`에 `myproc() != 0` 검사가 없었음(스레드가 항상 RUNNABLE이면 숨음). 6단계: `uvmalloc`/`uvmcopy`의 `mappages`에 `kalloc()` VA를 그대로 넘김(PTE에 `0xffffffff8...`, bit 63이 XD로 읽힘), `freewalk` 재귀에서 `P2V(child)` 빠뜨림(PA를 역참조), `uvmcreate`에서 `pagetable = cr3`(포인터 변수 자체를 덮음, 원본은 `kernel_pagetable`).

## 다음 할 일

원칙:
- 처음에는 `-smp 1`로만 돌린다. SMP는 sh 프롬프트가 뜬 다음에 한다.
- Makefile에는 포팅이 끝난 파일만 넣는다. 단계마다 부팅되는 상태로 커밋한다(중간 WIP 커밋은 OBJS 밖 파일이면 허용).
- qemu는 `-d int,cpu_reset -D qemu.log -no-reboot -no-shutdown`으로 돌린다. 타이머 때문에 tick마다 레지스터 덤프가 나오므로 화면 대신 `qemu.log`로 보낸다(`make clean`이 지움).

| 단계 | 할 일 | 확인 방법 |
|---|---|---|
| 0~3 | 완료 | |
| 4 | 완료(`22b9b6b`~`738e027`) | 타이머 tick ≈100Hz ✓ |
| 5 | 완료(`069f112`~`7936650`) | 커널 스레드 두 개가 선점으로 번갈아 돈다 ✓ |
| 6 | 완료(이 커밋) | `uvm_test: ok` ✓ |
| **7 (다음)** | initcode(`int $64`), IDT 64번 DPL=3, `forkret` → `trapret`, `syscall.c` argraw, `exec.c`(flags2perm → XD, `tf->rip/rsp`), `usys.pl`, uart.c·console.c·ide.c | `exec("/init")` |
| 8 | sh, usertests, 그다음 SMP(CPU 목록: ACPI MADT 또는 MP 테이블 결정, `entryother.S`, `startothers`) | |

6단계에서 7단계로 넘긴 것: proc.c `proc_pagetable`/`proc_freepagetable`(트램폴린·trapframe 매핑을 빼면 `uvmcreate`/`uvmfree`만 남음), `scheduler`의 cr3 전환(`swtch` 전 `write_cr3(V2P(p->pagetable))`, 돌아오면 커널 테이블), 현재 cr3에서 매핑을 지울 때의 TLB 무효화(riscv는 유저 복귀마다 satp+sfence로 비워졌지만 x86은 아님. `sys_sbrk` 축소 경로가 생길 때 x86.h에 `invlpg` 추가), exec `flags2perm`(ELF 플래그 → `PTE_RW`/`PTE_XD`, `uvmalloc`의 xperm).

4단계에서 정한 것: mp.c(CPU 목록)는 SMP용이라 8단계로 미뤘다. LAPIC/IOAPIC 주소는 qemu pc의 고정값 `LAPIC_PA`/`IOAPIC_PA`를 쓴다. spurious 벡터 `T_SPURIOUS`는 아직 trap()에서 따로 처리하지 않는다(오면 show_regs 후 멈춤. SDM상 spurious에는 EOI를 보내지 않는다).

5단계 세부:

| 세부 | 할 일 | 확인 |
|---|---|---|
| 5-a ✓ | 커널 스택 `proc_mapstacks` (`069f112`) | `info mem` |
| 5-b ✓ | TSS, per-CPU GDT, `ltr` (`014bec5`). 2026-10-30부터 사용자 요청으로 힌트를 구체적으로(값·비트 위치까지, 코드는 빼고) 줬다 | `TR`, GDT 메모리의 busy 비트 |
| 5-c ✓ | `swtch.S` (`830844c`), `procinit`/`allocproc` (`689ffd7`). 사용자는 `TEST(5-c-1)`, "5-c-3에서 forkret 채움"처럼 5-c 아래를 나눠 쓰고 있다 | `allocproc_test` 출력 |
| 5-d ✓ | `myproc`, x86.h `safe_halt()`, `scheduler`(사용자), `sched`/`yield`/`sleep*`/`wakeup`(Claude가 기계적 포팅), `forkret`(사용자), TEST(5) 스레드(Claude) | 직접 `yield`로 `abab` |
| 5-e ✓ | `tickslock`/`trapinit`, CPU 0만 ticks, EOI는 모두, `trap()` 끝에서 타이머면 `yield()`(`myproc() != 0`) | 선점으로 섞임, sleep 50 tick 테스트로 hlt 경로 |

TSS를 5단계에 넣은 이유(2026-10-27 결정): TSS는 CPL 3 → 0 전환 때만 쓰이지만, 7단계로 미루면 첫 유저 진입에서 틀렸을 때 출력 없이 triple fault가 나고 의심할 것이 너무 많다. 지금은 30줄 정도이고 `ltr`/`TR`로 바로 확인된다. 사용자 원칙: "필요한 것만"이 기본이지만, 나중에 하면 훨씬 힘든 것은 지금 한다.

## 함정

- cr3 수명: 지금 cr3에 실린 페이지테이블은 해제하지 않는다. 스케줄러가 프로세스에서 빠질 때 커널 테이블로 돌아가야 한다.
- `freewalk`가 상위 절반을 건드리면 커널 페이지테이블이 해제된다.
- PTE와 cr3에는 반드시 `V2P`한 값을 넣고, PTE에서 꺼낸 주소는 `P2V`해서 역참조한다. KSTACK과 디바이스 주소에는 V2P를 쓸 수 없다.
- `walk`에서 `va >= MAXVA` 검사는 커널 주소에서 panic한다. 유저 주소 검사는 walkaddr/copyin 쪽에서 한다.
- 유저 cs/ss에는 `|3`을 붙인다. IDT 64번이 DPL=3이 아니면 첫 시스템콜에서 #GP가 난다.
- 중간 페이지테이블 엔트리에 U 비트가 빠지면 유저 #PF가 난다(CPU는 모든 레벨의 권한을 AND).
- TSS 디스크립터는 16바이트(GDT 두 슬롯)이고 CPU마다 하나씩 필요하다(그래서 GDT도 CPU마다). `tss.sp0`는 컨텍스트 스위치마다 갱신한다. `ltr`은 디스크립터를 busy(type 0xB)로 바꾸므로 같은 디스크립터로 두 번 `ltr`하면 #GP. qemu(TCG)의 `info registers`는 `ltr` 후에도 TR을 `TSS64-avl`(0x89)로 보여 준다(캐시를 먼저 채우고 busy는 메모리에만 씀). busy 확인은 `x /2gx`로 GDT 메모리를 본다.
- `struct x86_hw_tss`는 packed여야 `sp0`가 오프셋 4에 온다. 빠지면 8로 밀리고, 7단계 첫 유저 트랩에서 출력 없이 triple fault가 난다.
- 비트 조립에서 `1 << 47`, `0x9 << 40`처럼 int 상수를 32 이상 밀면 `-Werror=shift-count-overflow`. `(uint64)` 캐스트나 `L` 접미사.
- 인터럽트마다 LAPIC EOI를 하지 않으면 다음 인터럽트가 오지 않는다. EOI는 `yield()`보다 먼저 한다(ISR에 벡터가 남은 채 다른 스레드로 가면 그 스레드에 타이머가 안 온다). `cpuid() == 0` 조건은 ticks에만 붙인다.
- `hlt`는 IF=0이면 영원히 깨어나지 않는다(riscv `wfi`는 SIE=0이어도 깨어남). 대기는 `safe_halt()`(asm 하나에 `sti; hlt`)로 한다. `sti`는 다음 명령 하나가 끝날 때까지 인터럽트를 보류하므로 `sti; hlt` 사이에 틈이 없고, 같은 이유로 riscv의 `intr_on(); intr_off();`(`sti; cli`)는 x86에서 인터럽트를 하나도 받지 않는다. 그래서 스케줄러 루프 위는 `intr_off()`만 둔다(Linux `do_idle()` 방식).
- 트랩 진입점이 하나라 스케줄러의 `safe_halt()` 중에 온 타이머도 `trap()`으로 온다. 그때 `myproc() == 0`이므로 `yield()` 전에 검사한다.
- 스케줄러가 인터럽트를 끈 채 `p->lock`을 잡으므로 `forkret`에서 풀어도 IF=0이다. 커널 스레드는 직접 `intr_on()`해야 선점된다(7단계 이후 유저 모드는 `iretq`의 rflags로 IF=1).
- 커널 스택은 프로세스마다 4KB 한 페이지(riscv xv6와 같음, Linux는 16KB). 넘치면 guard 페이지에서 #PF가 나지만 IST가 없어 같은 스택에 push하다 #DF → 출력 없이 triple fault. 이유 없는 triple fault는 `-d int`에서 `v=0e` 직후 `v=08`이고 cr2가 어떤 `KSTACK(p)` 바로 아래면 스택 넘침이다. 큰 지역 배열과 깊은 재귀를 피한다.
