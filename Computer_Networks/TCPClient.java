import java.io.*;
import java.net.*;

// TCP 소켓 클라이언트: localhost:6789 서버에 연결하여 키보드로 입력한 문자열을
// 보내고, 서버가 응답으로 보낸 학번을 받아 출력한 뒤 종료한다.
class TCPClient {
    public static void main(String argv[]) throws Exception {
        String sentenceForServer;  // Client에서 입력하여 Server로 보낼 문장을 담을 변수
        String sentenceFromServer; // Server에서 보낸 문자를 담을 변수

        BufferedReader inFromUser = new BufferedReader(new InputStreamReader(System.in));
        // 사용자(키보드)로부터 data를 받기 위한 InputStream 생성

        Socket clientSocket = new Socket("localhost", 6789);
        // Server에 연결을 시도하는 Client socket 생성.
        // 접속하려는 Server의 IP주소와 포트번호를 넣어야 하지만, Server를 local에서 돌리기 때문에 localhost를 넣음
        // localhost는 이 컴퓨터 자신을 의미하는 약속된 이름
        System.out.println("socket 생성하여 연결 요청.");

        DataOutputStream outToServer = new DataOutputStream(clientSocket.getOutputStream());
        // Server로 data를 보낼 OutputStream 생성

        BufferedReader inFromServer = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
        // Server로부터 data를 받기 위한 InputStream 생성

        System.out.print("전송할 문자열 입력 : ");
        sentenceForServer = inFromUser.readLine();
        // Server로 보낼 문자열을 사용자(키보드)로 직접 입력하며 저장

        outToServer.writeBytes(sentenceForServer + "\n");
        // 사용자(키보드)로 입력받은 문자열을 Server로 보냄
        System.out.println("입력 받은 문자열 Server로 전송");

        sentenceFromServer = inFromServer.readLine();
        // Server에서 보낸 data를 읽음
        System.out.println("Server로부터 받은 data : " + sentenceFromServer + "\n");

        System.out.println("Client를 종료합니다.");
        clientSocket.close();
    }
}
