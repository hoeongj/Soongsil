import java.util.Scanner;

public class travel {
    public static void main(String[] args) {
        Scanner s = new Scanner(System.in);

        System.out.print("여행지>>");
        String place = s.nextLine();

        System.out.print("인원수>>");
        int num = s.nextInt();

        System.out.print("숙박일>>");
        int day = s.nextInt();

        System.out.print("1인당 항공료>>");
        int plane = s.nextInt();

        System.out.print("1방 숙박비>>");
        int room = s.nextInt();

        System.out.print(num+"명의 "+place+" "+day+"박 "+(day+1)+"일 여행에는 방이 "+(num/2+num%2)+"개 필요하며 경비는 "+(room*(num/2+num%2)*day+plane*num)+"원입니다.");
        s.close();
    }
}
