import java.util.Scanner;
public class pro6_strRotation {
    public static void main(String[] args) {
        System.out.println("문자열을 입력하세요. 빈 칸이 있어도 되고 영어 한글 모두 됩니다.");
        Scanner sc = new Scanner(System.in);
        String s = sc.nextLine();
        for(int i = 0 ; i < s.length() ; i++){
            s = s.substring(1) + s.charAt(0);
            System.out.println(s);
        }
    }
}