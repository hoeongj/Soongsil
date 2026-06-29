import java.io.*;
import java.net.*;

// UDP 소켓 클라이언트: 키보드로 입력한 이름을 localhost:9876 서버로 보내고,
// 서버가 응답으로 보낸 학번을 받아 출력한 뒤 종료한다.
class UDPClient {
    public static void main(String args[]) throws Exception {
        BufferedReader inFromUser = new BufferedReader(new InputStreamReader(System.in));
        // 사용자(키보드)로부터 data를 받기 위한 InputStream 생성

        DatagramSocket clientSocket = new DatagramSocket();
        System.out.println("UDP 클라이언트 소켓 생성 완료.");
        // data를 주고받을 DatagramSocket 생성 (포트 번호는 OS가 자동으로 할당)

        InetAddress IPAddress = InetAddress.getByName("localhost");
        // 접속할 Server의 IP 주소를 InetAddress 객체로 변환
        // Server를 local에서 돌리기 때문에 localhost를 넣음

        byte[] sendData = new byte[1024];
        byte[] receiveData = new byte[1024];
        // Server로 data를 보내거나 Server로부터 data를 받기 위한 byte 배열 선언

        System.out.print("서버로 전송할 이름 입력 : ");
        String name = inFromUser.readLine();
        // 사용자(키보드)로부터 이름을 입력받아 String 변수에 저장

        sendData = name.getBytes();
        // 입력받은 이름을 byte 배열로 변환

        DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, 9876);
        // Server로 보낼 DatagramPacket(이름이 담긴 상자) 생성

        clientSocket.send(sendPacket);
        System.out.println("입력 받은 이름 Server로 전송");
        // Server로 이름 data를 전송

        DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
        // Server로부터 data를 수신하기 위한 DatagramPacket(빈 상자) 생성

        clientSocket.receive(receivePacket);
        // Server로부터 응답(학번)이 올 때까지 대기하다가, 받으면 receivePacket에 저장

        String studentIdFromServer = new String(receivePacket.getData());
        // 수신한 byte data를 String으로 변환

        System.out.println("서버로부터 받은 학번 : " + studentIdFromServer.trim());
        // Server로부터 받은 학번 출력

        System.out.println("Client를 종료합니다.");
        clientSocket.close();
        // socket을 닫고 프로그램을 종료
    }
}
