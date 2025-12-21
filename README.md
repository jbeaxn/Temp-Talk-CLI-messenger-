# 🎀 Temp-Talk 🍀 임시조직을 위한 CLI 메신저

> 💖 **[ELEC462] System Programming Final Project - Team 22 (Lab22)**
> 
> "Project ID와 Role만으로 접속하고,
>
> 프로젝트 종료 시 사라지는 임시조직용 메신저 💌"
>
> **데모 영상 ->** [https://youtu.be/fel9qtXzSGY?si=7Lz0DZqh9CE-qY4P](https://youtu.be/VXAN0Rs5ido?si=0lk7-uB_ACtFS39j)
> <img width="664" height="307" alt="image" src="https://github.com/user-attachments/assets/fe29c8ac-b3fb-4bea-83d3-8bdb550279ca" />


---

## 프로젝트 소개
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
| **신정빈** | 2024004723 | Client / Design | • Client 코드 구현<br>• UI/UX + README 🍀<br>• 데모영상 제작 💖 |

**협업 방법:**  
- Server/Client 분리 개발 후 통합
- 코드 실시간 공유 + 상호 피드백으로 디버깅

---

## 기술 & 구현 

### 개발 환경
* **Language:** C (900+ Lines of Code)
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
(데모 영상에서 모두 시연)

1. **휘발성 메시지 `/bomb [초] [메시지]` 💣**
   - 일정 시간 후 메시지 자동 삭제
   - 메시지 옆에 남은 시간 표시
   - <img width="233" height="44" alt="image" src="https://github.com/user-attachments/assets/03cedf49-1d3d-44aa-b051-60f2297886c3" />
   - <img width="218" height="38" alt="image" src="https://github.com/user-attachments/assets/c1e29423-2af6-447d-bdb1-7b647dc30048" />
   

2. **파일 관리 `/upload [파일명]`, `/list` 📁**
   - 서버에 파일 업로드, 목록 확인 
   - upload test.txt 로 확인하실 수 있습니다
   - <img width="381" height="97" alt="image" src="https://github.com/user-attachments/assets/f784483d-a62f-4654-a289-a1d3c202c62b" />


3. **파일 바로보기 `/open [파일명]` 👀**
   - 다운로드 없이 즉시 확인  (쉘에 즉시 렌더링!)
   - <img width="446" height="96" alt="image" src="https://github.com/user-attachments/assets/d0eab7b0-8da8-4478-9d82-ec84dca3273f" />


4. **미니 게임 `/game` 🎮**
   - 팀원 간 아이스브레이킹을 위한 타이핑 게임 시작
   - <img width="320" height="318" alt="image" src="https://github.com/user-attachments/assets/71b016ec-8fe9-490d-912d-e3e27c39d6e5" />


5. **프로젝트 만료일 설정 `/expire [만료일]` ⏰**
   - 설정된 만료 시간 이후 서버 데이터 완전 삭제 & 방 폭파
   - /expire 0 으로 간단히 테스트 해보실 수 있습니다 (만료일 0으로 설정)
   - <img width="185" height="88" alt="image" src="https://github.com/user-attachments/assets/e06e2a3d-31eb-4dca-b55d-05e4765216a1" />
   - <img width="440" height="179" alt="image" src="https://github.com/user-attachments/assets/d41ddfcf-9007-4813-899a-95bf0d4da061" />



6. **명령어 설명(도움말) `/help` ❓**
   - 명령어에 대한 설명 및 명령어 목록 조회
   - 엔터를 누르면 다시 채팅 화면으로 돌아가도록 구현
   - <img width="349" height="216" alt="image" src="https://github.com/user-attachments/assets/8f2bbb0c-8803-40c0-84b3-b9656a87060a" />


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
