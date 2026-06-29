import java.util.Scanner;

public class birth {
    public static void main(String[] args) {
        int birth;
        Scanner s = new Scanner(System.in);
        System.out.print("생일 입력 하세요>>");
        birth = s.nextInt();
        System.out.print(birth/10000+"년 "+birth/100%100+"월 "+birth%100+"일");
        s.close();
    }
}