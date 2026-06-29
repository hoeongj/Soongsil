class Cube{
    private int width;
    private int length;
    private int height;
    Cube(int width, int length, int height){
        this.width = width;
        this.length = length;
        this.height = height;
    }
    int getVolume(){
        return width*length*height;
    }

    void increase(int width, int length, int height){
        this.width+=width;
        this.length+=length;
        this.height+=height;
    }

    boolean isZero(){
        return width==0 && length==0 && height==0;
    }
}

public class pro2 {
    public static void main(String[] args) {
        Cube cube = new Cube(1,2,3);
        System.out.println("큐브의 부피는 " + cube.getVolume());
        cube.increase(1,2,3);
        System.out.println("큐브의 부피는 " + cube.getVolume());
        if(cube.isZero()) System.out.println("큐브의 부피는 0");
        else System.out.println("큐브의 부피는 0이 아님");
    }
}