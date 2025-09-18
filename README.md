# fru1t OS

RISC-V 32비트 아키텍처용 C언어로 작성된 최소한의 운영체제 커널, 키보드 인터럽트 처리와 대화형 셸 기능을 구현

## 개발 도움

이 프로젝트는 OpenSBI 통합 부분에서 Claude AI의 지원을 받음

## 기능

- **부트 프로세스**: OpenSBI 통합이 포함된 커스텀 부트로더 진입점
- **메모리 관리**: kmalloc/kfree를 사용한 동적 힙 할당자 (1MB 힙)
- **프로세스 스케줄링**: 최대 8개 프로세스를 지원하는 라운드로빈 스케줄러
- **파일 시스템**: 32개 파일 슬롯을 가진 인메모리 파일시스템 (파일당 최대 1KB)
- **트랩 처리**: 완전한 예외 및 인터럽트 처리 시스템
- **키보드 입력**: 인터럽트 및 폴링 모드를 지원하는 UART 기반 키보드 입력
- **대화형 셸**: 9개의 내장 명령어를 가진 명령줄 인터페이스
- **입력 버퍼**: 키보드 입력 처리를 위한 순환 버퍼

## 셸 명령어

운영체제에는 다음 명령어들을 지원하는 완전한 기능의 셸이 포함되어 있음:

- `help` - 사용 가능한 명령어 보기
- `ls` - 파일시스템의 파일 목록 보기
- `cat <filename>` - 파일 내용 표시
- `create <filename> <size>` - 새 파일 생성
- `delete <filename>` - 파일 삭제
- `echo [text]` - 텍스트를 콘솔에 출력
- `memstat` - 메모리 할당 통계 보기
- `clear` - 화면 지우기
- `exit` - 셸 종료

## 필수 요구사항

### QEMU
```bash
# macOS
brew install qemu

# Ubuntu/Debian
sudo apt-get install qemu-system-misc

# Arch Linux
sudo pacman -S qemu-arch-extra
```

### 컴파일러
```bash
# macOS (권장)
brew install llvm

# Ubuntu/Debian
sudo apt-get install clang
```

## 빌드 및 실행

### 빠른 시작
```bash
git clone <repository-url>
cd myos
chmod +x run.sh
./run.sh
```

### 수동 빌드
```bash
CC=/opt/homebrew/opt/llvm/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c common.c

qemu-system-riscv32 \
    -machine virt \
    -bios default \
    -nographic \
    --no-reboot \
    -kernel kernel.elf
```

## 예상 출력

```
OpenSBI v1.5.1
...
메모리 할당자 초기화 중...
메모리 할당자 초기화 완료: 1048576 바이트 사용 가능
파일시스템 초기화 중...
파일시스템 초기화 완료: 32개 파일 슬롯 사용 가능
UART 및 키보드 인터럽트 초기화 중...
UART 초기화 완료
샘플 파일 생성 중...
파일 'welcome.txt' 생성 완료 (256 바이트)
파일 'welcome.txt'에 21 바이트 작성
파일 'readme.txt' 생성 완료 (512 바이트)
파일 'readme.txt'에 66 바이트 작성
셸 데모 시작 (키보드 입력은 시뮬레이션됨)...

=== Fru1t OS 셸 데모 ===
(키보드 입력이 구현되지 않아 사용자 명령을 시뮬레이션)

fru1t-os> help

=== Fru1t OS 셸 명령어 ===
help          - 도움말 메시지 표시
ls            - 파일시스템의 파일 목록
cat <file>    - 파일 내용 표시
create <file> <size> - 새 파일 생성
delete <file> - 파일 삭제
echo [args]   - 인자 출력
memstat       - 메모리 통계 표시
clear         - 화면 지우기
exit          - 셸 종료

fru1t-os> ls

=== 파일 시스템 목록 ===
파일: 2/32
  welcome.txt (256 바이트)
  readme.txt (512 바이트)

fru1t-os> cat welcome.txt
파일 'welcome.txt'에서 256 바이트 읽음
welcome.txt의 내용:
Fru1t OS에 오신 것을 환영합니다!

...
```

## 아키텍처

### 메모리 레이아웃
- **로드 주소**: 0x80200000
- **스택**: 프로세스당 64KB
- **힙**: 동적 할당을 위한 1MB
- **최대 파일 수**: 32개 파일, 각각 1KB

### 주요 구성요소

#### 프로세스 관리
- 10ms 타임 슬라이스를 가진 라운드로빈 스케줄러
- 프로세스 상태: UNUSED, READY, RUNNING, BLOCKED
- 레지스터 보존을 포함한 완전한 컨텍스트 스위칭

#### 메모리 할당자
- First-fit 할당 알고리즘
- 블록 분할 및 병합
- 8바이트 정렬된 할당
- 메모리 누수 감지

#### 파일 시스템
- 동적 할당을 사용한 인메모리 저장소
- 파일 연산: 생성, 읽기, 쓰기, 삭제, 목록
- 파일명 제한: 64자
- 파일 크기 제한: 1024바이트

#### 입력 시스템
- UART 기반 키보드 입력 처리
- 인터럽트 및 폴링 모드 지원
- 순환 입력 버퍼 (256바이트)
- 실시간 명령 처리

## 프로젝트 구조

```
.
├── kernel.c        # 메인 커널 구현
├── kernel.h        # 커널 헤더 및 정의
├── common.c        # 유틸리티 함수 (printf, memset 등)
├── common.h        # 공통 헤더
├── kernel.ld       # 메모리 레이아웃용 링커 스크립트
├── run.sh          # 빌드 및 실행 스크립트
├── test_input.sh   # 키보드 입력 테스트 스크립트
└── README.md       # 이 문서
```

## 개발

### 새 명령어 추가
1. `kernel.c`에 명령어 핸들러 함수 추가
2. `shell_execute_command()`에 새 케이스 업데이트
3. `cmd_help()`에 도움말 텍스트 추가

### 메모리 관리
- 동적 할당을 위해 `kmalloc()`과 `kfree()` 사용
- 누수 디버깅을 위해 `print_memory_stats()` 확인
- `HEAP_SIZE` 정의를 통해 힙 크기 설정 가능

### 파일 시스템 확장
- 다른 제한을 위해 `MAX_FILES`와 `MAX_FILESIZE` 수정
- 파일시스템 섹션에 새로운 파일 연산 추가
- 파일은 RAM에 저장되며 재부팅시 손실됨

## 제어

- **QEMU 종료**: Ctrl+A, X
- **QEMU 모니터**: Ctrl+A, C
- **셸 탐색**: 표준 터미널 제어

## 알려진 문제

- 일부 QEMU 설정에서 키보드 입력이 제대로 작동하지 않을 수 있음
- 파일이 영구적이지 않음 (RAM 전용 저장소)
- 32비트 RISC-V 아키텍처로 제한됨
- 가상 메모리 관리 없음

## 라이선스

MIT 라이선스 - 자유롭게 사용하고 수정 가능

## 참고자료

- [RISC-V 명령어 세트 매뉴얼](https://riscv.org/technical/specifications/)
- [OpenSBI 문서](https://github.com/riscv-software-src/opensbi)
- [QEMU RISC-V 문서](https://www.qemu.org/docs/master/system/target-riscv.html)
