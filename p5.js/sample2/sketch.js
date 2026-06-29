function setup() {
  createCanvas(600, 400);
}

function draw() {
  background(240);

  let handY = 450; 
  let lipH = 10; 
  let grillzH = 2; 

  fill(20); noStroke(); ellipse(300, 450, 450, 300);  
  fill(255, 235, 215); rect(275, 280, 50, 40); 
  stroke(0); strokeWeight(1); ellipse(300, 180, 200, 240); 
  fill(10); arc(300, 160, 210, 200, PI, TWO_PI);  
  fill(255, 235, 215); ellipse(190, 210, 30, 50); ellipse(410, 210, 30, 50);  
  fill(255); ellipse(260, 170, 35, 20); ellipse(340, 170, 35, 20);  
  fill(0); circle(260, 170, 15); circle(340, 170, 15);
  noFill(); stroke(150, 100, 50, 100); arc(300, 210, 20, 10, 0, PI); 

  stroke(0); fill(50); strokeWeight(2);
  let lipY = 235 - (lipH - 10) / 2; 
  rect(245, lipY, 110, lipH, 10);

  strokeWeight(1); stroke(0, 50);
  let upperY = lipY + 3; 
  let lowerY = lipY + lipH - grillzH - 3; 
  for (let i = 0; i < 4; i++) { 
    let x = 250 + i * 25;
    fill(220, 230, 240); rect(x, upperY, 25, grillzH);
    noStroke(); fill(255); circle(x + 25 * 0.3, upperY + grillzH * 0.3, 4); circle(x + 25 * 0.7, upperY + grillzH * 0.7, 2);
    stroke(0, 50); strokeWeight(1);
  }
  for (let i = 0; i < 4; i++) { 
    let x = 250 + i * 25;
    fill(220, 230, 240); rect(x, lowerY, 25, grillzH);
    noStroke(); fill(255); circle(x + 25 * 0.3, lowerY + grillzH * 0.3, 4); circle(x + 25 * 0.7, lowerY + grillzH * 0.7, 2);
    stroke(0, 50); strokeWeight(1);
  }

  fill(255, 220, 190); stroke(0); strokeWeight(1);
  rect(195, handY, 50, 30, 10); 
  rect(355, handY, 50, 30, 10); 

  fill(200); noStroke();
  circle(188, 200, 8); circle(188, 215, 8); circle(188, 230, 8);
  circle(412, 200, 8); circle(412, 215, 8); circle(412, 230, 8);
}