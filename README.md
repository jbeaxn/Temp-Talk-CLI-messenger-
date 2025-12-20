# 🎀 Temp-Talk 🍀 임시조직을 위한 CLI 메신저

> 💖 **[ELEC462] System Programming Final Project - Team 22 (Lab22)**
> 
> "Project ID와 Role만으로 접속하고,
>
> 프로젝트 종료 시 흔적 없이 사라지는 임시조직용 메신저 💌"
>
> 데모 영상 -> https://youtu.be/fel9qtXzSGY?si=7Lz0DZqh9CE-qY4P

---

## 프로젝트 소개 (Overview)
* **프로젝트명:** Temp-Talk 🎀  
* **팀명:** Lab22 (Team 22) 🍀
* **개발 배경:**
  - 💬 기존 메신저 사용 시 공적인 상황에서 사생활 노출 (+ 최근 카카오톡 이슈)
  - 🗂 프로젝트 종료 후에도 데이터가 남아 민감 정보 유출 위험
* **해결 방법:**
  - 프로필 없이 접속 (Project ID + Role)
  - 프로젝트 종료 시 데이터 완전 삭제 💣

---

## 팀원 및 역할

| 이름 | 학번 | 역할 | 담당 파트 💡 |
|:---:|:---:|:---:|:---|
| **노현아** | 2024003948 | Server / Debugging | • Server 코드 구현<br>• 오류 디버깅<br>• PPT 제작 🎀 |
| **신정빈** | 2024004723 | Client / Design | • Client 코드 구현<br>• UI/UX + README 🍀<br>• 발표 💖 |

**협업 방법:**  
- Server/Client 분리 개발 후 통합
- 코드 실시간 공유 + 상호 피드백으로 디버깅

---

## 기술 & 구현 

### 개발 환경
* **Language:** C (812+ Lines of Code)
* **Communication:** TCP/IP Socket
* **Concurrency:** POSIX Threads (pthread), Mutex

### 시스템 콜 & 라이브러리
* **File I/O:** `open`, `read`, `write`, `close`  
* **Network:** `socket`, `bind`, `listen`, `accept`, `connect`, `setsockopt`  
* **Signal & Process:** `exit`, `signal`, `alarm`, `time`  

### 더 나아가서
* Multi-threading: 여러 클라이언트 동시 접속  
* Socket Programming: 채팅방 입장 & 명령어 데이터 전송  
* Synchronization: Mutex 활용 데이터 안전 처리  

---

## 📎주요 기능(다양한 커맨드들 . .)
사용자는 `/` 명령어로 다양한 기능을 사용할 수 있습니다

타 CLI 메신저와의 가장 큰 차별점입니다

1. **휘발성 메시지 `/bomb [초] [메시지]` 💣**
   - 일정 시간 후 메시지 자동 삭제  

2. **파일 관리 `/upload [파일명]`, `/list` 📁**
   - 서버에 파일 업로드, 목록 확인  

3. **파일 바로보기 `/open [파일명]` 👀**
   - 다운로드 없이 즉시 확인  

4. **미니 게임 `/game` 🎮**
   - 팀원 간 아이스브레이킹

5. **프로젝트 만료일 설정 `/expire [만료일]` ⏰**
   - 설정된 만료 시간 이후 서버 데이터 완전 삭제 & 방 폭파
   - /expire 0 으로 간단히 테스트 해보실 수 있습니다 (만료일 0으로 설정) 

6. **명령어 설명(도움말) `/help` ❓**
   - 명령어에 대한 설명 및 명령어 목록 조회

7. **채팅방 종료 `/exit`🚪**
   - 채팅방 종료하고 싶을 때 종료

## 설치 & 실행 방법 🚀

### 
```bash
# 경로 이동
cd Temp-Talk-CLI-messenger-
cd TmpTalk

# 컴파일
make

# 서버
./server

# 클라이언트 (새 wsl열고)
./client

# 로그인 (클라이언트 창에서)
프로젝트명 + 역할명으로 접속

# 종료
/exit or Ctrl+C
