import java.util.Scanner;
public class pro10_strQuiz_2 {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        String[] arr = {"happy","morning","package","together","hello"};
        System.out.println("10초 안에 단어를 맞추세요!!");
        int indx1;
        int indx2;
        char tmp;
        char[] arr2;
        String ans;
        for(int i = 0 ; i < arr.length ; i++){
            arr2 = arr[i].toCharArray();
            for(int j = 0 ; j < arr[i].length() ; j++) {
                indx1 = (int)(Math.random()*arr[i].length());
                indx2 = (int)(Math.random()*arr[i].length());
                tmp = arr2[indx1];
                arr2[indx1] = arr2[indx2];
                arr2[indx2] = tmp;
            }
            for(int j = 0 ; j < arr2.length ; j++) System.out.print(arr2[j]);
            System.out.print("\n>>");
            double start = System.currentTimeMillis();
            ans = sc.nextLine();
            double end = System.currentTimeMillis();
            double time = (end-start)/1000;
            if(ans.equals("그만")) return;

            if(!ans.equals(arr[i])){
                System.out.print("실패!!! "+arr[i]+" 입니다.");
                if(time >= 10) System.out.print(" 10초 초과");
            }
            else{
                if(time >= 10) System.out.print("실패!!! 10초 초과");
                else System.out.print("성공!!!");
            }
            System.out.printf(" %.3f",time);
            System.out.println("초 경과");
        }
    }
}