Overlapped3(IOCP) 에코서버
gw14 IOCP 전이중 (프로파일 없이)

hw_45_1
ㄴmyOverlapped 구조체에서 socketinfo 빼고 sendflag SOCKETINFO구조체 멤버변수로 넣어둠..정리해둠
ㄴ제대로 IOCP 전이중 만들어둔게 이거

hw_45_2
ㄴsessionmap에 session 보관
ㄴaccept스레드 따로
ㄴ직렬화버퍼 넣어둠

hw_45_3
ㄴ링버퍼 소멸자 오류 떠서 세션 지우는 타이밍 문제라고 생각...
 ㄴ그래서 remove,sendcompletion, recvcompletion플래그를 잡고 이 3개 플래그가 모두 세워지면 지우기...
  ㄴ잘 안 지워짐...하고 있음....

hw_45_4
ㄴ나도 Send시 버퍼 2개 꽂으면 errorcode 10022가 뜸.... ㄴsend버퍼 하나만 꽂으면 문제 없긴한데...왜?????????
ㄴ나도 printf들 지우니까...WSASend 걸 때 10022error 뜸..
 ㄴ링버퍼 SendQ의 경우에는 계속 recv받으면서 넣어주다보니 계속 rear가 바뀜...과거값으로 하여 버퍼 2개 꽂으려면 그때 getusedsize가 front부터 버퍼 끝까지의 크기보다 큰지 비교 후 크다면 버퍼 2개 넣고 아니라면 1개만 넣고...조건들 비교할 게 많음...
  ㄴ그래서 일단 그냥 directdequeuesize 하나만 보내주고 있음..
