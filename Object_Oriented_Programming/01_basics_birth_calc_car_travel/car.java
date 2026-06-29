import java.util.Scanner;

public class car {
    public static void main(String[] args) {
        Scanner s = new Scanner(System.in);
        System.out.print("자동차 상태 입력>>");
        int car = s.nextInt();
        int temp = car%32;
        String aircon;
        String gostop;
        if(car/64%2 == 0) aircon = "꺼진";
        else aircon = "켜진";
        if(car/128%2 == 0) gostop = "정지";
        else gostop = "달리는";

        System.out.print("자동차는 "+gostop+" 상태이고 에어컨이 "+aircon+" 상태이고 온도는 "+temp+"도이다.");

        s.close();
    }
}
