# CyberSecureExp_6

# 实验六：简单栈溢出实验

## 实验目的

本实验将编写受害程序，在操作系统环境下完成一次简单的栈区溢出攻击。本实验中提包含了如下步骤：

（1）设计编写受害程序。

（2）在关闭操作系统的防御机制的条件下编译受害程序。

（3）对受害程序进行反汇编，观察变量与函数的地址。

（4）构造恶意负载并输入程序。

## 实验环境设置

本实验的环境设置如下：

实验在常规的 Linux 环境下可完成，几乎对发行版本与内核版本没限制。但需要准备 GCC 等软件作为编译工具。事先需要一个受害程序：需要编写一个受害函数，包含一个不设长度检查的 gets 函数调用；并准备好一个函数作为攻击者希望恶意调用的函数，若程序正常执行不会调用该函数。

## 实验步骤

本实验的过程及步骤如下：

（1）在实验中练习关闭 Linux 相关的一系列保护机制，来体会内存保护机制的重要性。首先需要在编译阶段将编译目标指定为 32 位，关闭 Stack Canary保护，还需要关闭 PIE（Position Independent Executable），避免加载基址被打乱，此外还需要关闭系统环境下的地址空间分布随机化机制。然后，我们可以使用命令 gcc -v 查看 gcc 默认的开关情况。编译成功后，可以使用 checksec 工具检查编译出的文件。

（2）对受害程序进行反汇编，并分析汇编码。在一般情况下，攻击者拿到的是一个二进制形式的程序，因而需要先对其进行逆向分析。可以使用 IDA Pro这类专用的逆向分析工具来反汇编一下二进制程序并查看： ① 攻击者希望恶意调用的函数的地址，确定将覆盖到返回地址的内容； ② 接收 gets 函数输入的数组局部变量到被保存在栈中的 EBP 的长度，计算可越界访问变量到返回地址间的内存距离。考虑到 IDA Pro 为付费商业软件，使用 GDB 也可以完成相同的任务。

（3）编写一个攻击程序，将构造的恶意输入受害程序。然后是构造恶意输入，推荐使用 Python 的 pwntools 工具根据上述信息构造恶意输入。最后观察实验效果，判定攻击是否成功。

## 预期实验结果

受害程序从标准输入流获得用户的输入。首先保存 gets 函数输入的位于栈区的局部变量将被越界访问，攻击者希望恶意调用的函数的地址将被覆盖到栈帧中 EBP 位置后的返回地址。当函数返回时将返回至攻击者预期的函数中执行，证明试验成功；否则，程序正常执行并退出，或者程序直接崩溃，则证明实验失败。

## 代码实现说明

本实验最终在当前环境中实现了一个 **32 位** 栈溢出靶机程序 `vuln.c`，以及相应的利用脚本 `exploit.py`。为支持 32 位编译，实验过程中安装了 `gcc-multilib`、`libc6-dev-i386` 等 32 位开发库。

- 受害程序 `vuln.c`：
  - 定义了函数 `attacker_function`，正常执行流程不会调用该函数。一旦栈溢出成功并覆盖返回地址，将跳转到该函数中执行，打印“栈溢出成功：已经跳转到 attacker_function！”的提示，用于证明控制流已被劫持到攻击者指定的恶意函数。
  - 定义了存在漏洞的函数 `vulnerable`，其中包含一个栈上局部缓冲区 `char buf[64]`，并故意使用不安全的 `gets(buf)` 从标准输入读取数据，不进行长度检查，为栈溢出入口。`main` 函数打印实验提示后调用 `vulnerable`，若未被攻击则正常返回并打印“程序正常结束。”后退出。
- 利用脚本 `exploit.py`：
  - 使用 pwntools 的 `ELF("./vuln")` 自动解析可执行文件，获取目标函数 `attacker_function` 的实际地址（例如一次运行中的输出为 `attacker_function 地址: 0x8049196`），完成实验步骤（2）中“获取恶意函数地址”的要求。
  - 根据 32 位栈帧布局，通过 GDB 反汇编 `vulnerable` 得到：函数序言为 `push %ebp; mov %esp,%ebp; push %ebx; sub $0x44,%esp`，`gets` 使用的缓冲区地址为 `[ebp-0x48]`，而返回地址位于 `[ebp+4]`。因此从 `buf` 起始到返回地址的距离为 `0x48 + 4 = 0x4c` 字节，即 **76 字节**。
  - 在 `exploit.py` 中据此构造 payload：前 76 字节用 `"A"` 填充以覆盖 `buf`、保存的 `ebx` 和 `ebp`，随后用 4 字节写入 `attacker_function` 地址（`p32(attacker_addr)`），使返回地址被恶意函数地址替换。
  - 使用 `process("./vuln")` 启动受害程序，等待提示字符串 “请输入内容” 后发送 payload。若覆盖成功，当 `vulnerable` 返回时 EIP 将指向 `attacker_function`，程序打印出“栈溢出成功：已经跳转到 attacker_function！”这一行信息，证明攻击成功。

## 编译与运行

1. 编译与保护机制配置

按照实验要求，需要在编译阶段将目标指定为 32 位，并关闭多种保护机制。最终使用的编译命令为：

```bash
gcc -m32 -fno-stack-protector -z execstack -no-pie vuln.c -o vuln
```

各选项含义如下：

- `-m32`：将编译目标指定为 **32 位**；
- `-fno-stack-protector`：关闭栈溢出保护（Stack Canary）；
- `-no-pie`：关闭 PIE，使可执行文件加载基址固定，便于预测函数地址；
- `-z execstack`：将栈标记为可执行，更贴近传统栈溢出实验环境。

编译完成后，可通过 pwntools 的 `ELF("./vuln")` 自带的 `checksec` 输出检查结果，例如：

```text
[*] '/workspace/CyberSecureExp_6/vuln'
    Arch:       i386-32-little
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX unknown - GNU_STACK missing
    PIE:        No PIE (0x8048000)
    Stack:      Executable
    RWX:        Has RWX segments
    Stripped:   No
```

可以看到：

- **架构**为 `i386-32-little`，确认为 32 位程序；
- 栈保护 canary 已关闭（`No canary found`）；
- PIE 关闭（`No PIE (0x8048000)`），加载基址固定；
- 栈可执行（`Stack: Executable`）。

在系统层面尝试关闭 ASLR 时，执行：

```bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

容器返回 `tee: /proc/sys/kernel/randomize_va_space: Read-only file system`，说明该参数所在的文件系统是只读的，无法通过该方式修改。虽然无法从容器内完全关闭 ASLR，但由于本实验可执行文件关闭了 PIE，代码段地址稳定，`attacker_function` 的地址在多次运行中保持一致，实验仍可顺利完成。

2. 反汇编与偏移计算（GDB 实验）

为实现实验步骤（2），使用 GDB 对受害程序进行反汇编和栈帧分析：

- 反汇编关键命令：

```bash
gdb -q ./vuln -ex 'disassemble attacker_function' -ex 'disassemble vulnerable' -ex 'quit'
```

- GDB 显示 `attacker_function` 的入口地址为类似 `0x08049196`；`vulnerable` 的汇编中包含：
  - 入口处 `push %ebp`、`mov %esp,%ebp` 建立新的栈帧；
  - 紧接着 `push %ebx`，再 `sub $0x44,%esp` 为局部变量分配空间；
  - `lea -0x48(%ebp),%eax` 将 `buf` 的地址计算为 `[ebp-0x48]`，`gets` 从该地址开始写入。

由此可以得出：

- `buf` 起始地址：`[ebp-0x48]`；
- 返回地址：`[ebp+4]`。

从 `buf` 到返回地址的总偏移为：

- `0x48（从 buf 到 ebp）+ 4（从 ebp 到返回地址） = 0x4c = 76` 字节。

这与 `exploit.py` 中设置的 `offset = 76` 完全一致，实现了从汇编码逆推出溢出偏移的全过程。

3. 利用脚本与实验运行结果

- 环境依赖：

```bash
pip install pwntools
```

- 正常运行受害程序（不进行攻击）：

```bash
./vuln
```

按提示输入一段短字符串（如 `hello`），程序将输出：

```text
简单栈溢出实验程序
请输入内容：
收到输入：hello
程序正常结束。
```

此时未触发 `attacker_function`，说明在正常输入下程序行为安全。

- 使用利用脚本发起栈溢出攻击：

```bash
python3 exploit.py
```

pwntools 会首先输出 `attacker_function` 的地址、`checksec` 信息等，然后启动 `./vuln` 并发送构造好的 payload。一次典型攻击的关键输出为：

```text
[*] '/workspace/CyberSecureExp_6/vuln'
    Arch:       i386-32-little
    RELRO:      Partial RELRO
    Stack:      No canary found
    PIE:        No PIE (0x8048000)
    Stack:      Executable
    ...
[*] attacker_function 地址: 0x8049196
[+] Starting local process './vuln': pid xxxx
[*] Switching to interactive mode
：
收到输入：AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA��
栈溢出成功：已经跳转到 attacker_function！
[*] Got EOF while reading in interactive
```

可以看到，当溢出 payload 被写入后，`vulnerable` 返回不再回到调用点，而是跳转到了 `attacker_function`，并打印出专门用于标记攻击成功的提示信息。随后由于栈结构已被破坏，进程以 `SIGSEGV` 退出，这在栈溢出攻击实验中是预期现象，不影响对“控制流是否被成功劫持”的判断。

综上，本次实验按步骤依次完成了：安装 32 位开发环境并关闭关键内存保护机制、对受害程序进行 GDB 级别的反汇编与栈分析、使用 pwntools 编写攻击脚本构造精确 payload，并通过输出信息验证了攻击者指定的恶意函数被成功调用，达成了实验目标。
