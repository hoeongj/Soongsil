import java.util.Scanner;

public class pro16 {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        int sum = 0, count = 0;
        String input;
        System.out.print("양의 정수를 입력하세요. -1을 입력 끝>>");

        while (true) {
            input = sc.next();
            if(input.equals("-1")) break;
            try {
                int num = Integer.parseInt(input);
                if (num > 0) {
                    sum += num;
                    count++;
                }
                else System.out.println(input + " 제외");
            } catch (NumberFormatException e) {
                System.out.println(input + " 제외");
            }
        }
        System.out.print("평균은 " + (sum / count));
        sc.close();
    }
}
