import java.io.*;
import java.net.*;

// UDP 소켓 서버: 9876 포트에서 DatagramPacket을 수신해 클라이언트가 보낸 이름을
// 출력하고, 요청을 보낸 주소/포트로 학번(20221528)을 응답한 뒤 종료한다.
class UDPServer {
    public static void main(String args[]) throws Exception {
        DatagramSocket serverSocket = new DatagramSocket(9876);
        System.out.println("socket 생성 완료. port : 9876");
        // 9876 포트로 데이터를 주고받을 DatagramSocket 생성

        byte[] receiveData = new byte[1024];
        byte[] sendData = new byte[1024];
        // Client로부터 데이터를 받을 변수와 Client에게 데이터를 보내기 위한 배열 선언

        // 서버는 종료되지 않고 계속 Client의 요청을 기다림
        while (true) {
            DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
            System.out.println("데이터 수신 대기중");
            // Client로부터 데이터를 받기 위한 DatagramPacket 생성

            serverSocket.receive(receivePacket);
            System.out.println("데이터 수신 완료");
            // Client로부터 데이터가 올 때까지 대기하다가, 받으면 receivePacket에 저장

            String myName = new String(receivePacket.getData());
            // Client로부터 받은 데이터를 myName 변수에 저장

            InetAddress IPAddress = receivePacket.getAddress();
            int port = receivePacket.getPort();
            // 데이터를 보낸 Client의 IP 주소와 Port 번호를 얻어옴

            System.out.println("Client가 보낸 이름 : " + myName);
            // Client가 보낸 이름 출력

            String studentId = "20221528";
            sendData = studentId.getBytes();
            // Client에게 보낼 학번을 studentId 변수에 저장

            DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, port);
            // Client에게 데이터를 보내기 위한 DatagramPacket(학번이 담긴 상자) 생성

            serverSocket.send(sendPacket);
            System.out.println("Client로 학번이 전송됨");
            // Client에게 학번 데이터를 전송

            System.out.println("Server를 종료합니다.");
            serverSocket.close();
            break;
        }
    }
}
