import java.util.Scanner;

public class pro6 {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        int[] input = new int[10];
        int sum;
        int n;
        System.out.print("양의 정수 10개 입력>>");
        for (int i = 0; i < input.length; i++) input[i] = sc.nextInt();

        System.out.print("자리수의 합이 9인 것은 ...");
        for (int i = 0; i < input.length; i++) {
            sum = 0;
            n = input[i];
            while (n != 0) {
                sum += n % 10;
                n /= 10;
            }
            if (sum == 9) {
                System.out.print(input[i] + " ");
            }
        }
    }
}