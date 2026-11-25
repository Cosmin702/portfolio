// Tutti i contenuti del sito stanno qui. Modifica questo file, non index.html.
// Campi "todo": cose da completare (link, foto). Le card senza immagine mostrano un'icona.

const PROFILE = {
  name: "Cosmin",
  role: "Computer Engineering student · Embedded systems",
  roleZh: "计算机工程学生 · 嵌入式系统",
  bio: "I study Computer Engineering at Politecnico di Torino and I'm spending this year at Tongji University (EIE, Politong exchange program). I like building things where software meets hardware: custom PCBs, ESP32 networks, small robots and my own servers.",
  location: "Shanghai · Torino",
  links: {
    github: "https://github.com/Cosmin702",
    makerworld: "", // TODO: link al profilo MakerWorld, es. https://makerworld.com/en/@nomeutente
    email: "",      // TODO: email pubblica (facoltativa)
    linkedin: ""    // TODO (facoltativo)
  },
  skills: [
    "C / C++", "Python", "ESP32 / Arduino", "PCB design", "ESP-NOW · Wi-Fi",
    "Linux", "Docker · Proxmox", "Networking / VPN", "C# / .NET", "HTML · CSS · JS",
    "3D design & printing"
  ]
};

const PROJECTS = [
  {
    id: "hackathon",
    title: "Fall-check home robot",
    titleZh: "居家跌倒检测机器人",
    tag: "In progress · DDC 2026 Hackathon",
    category: "robotics",
    featured: true,
    image: "",
    summary: "A home robot for elderly people who live alone. When it hears a fall or a call for help, it goes to check, asks \"are you OK?\" and alerts the family on WeChat only if there's no answer. Everything runs locally on a D-Robotics RDK X5.",
    stack: ["RDK X5", "ROS 2 / TROS", "Pose detection", "Voice commands", "ESP32 wearable"],
    role: "Software lead in a team of 4",
    links: { repo: "" } // TODO: repo del progetto dopo l'hackathon
  },
  {
    id: "smart-gate",
    title: "Smart Gate",
    titleZh: "智能门控",
    tag: "Custom PCB · 2025",
    category: "embedded",
    featured: true,
    image: "assets/smart-gate.jpg",
    summary: "A custom PCB with an ESP32-C3 that sits on top of the original gate remote and presses its buttons, so the gate opens from my phone over the internet while the normal remote keeps working. My final high-school project, still running at home.",
    stack: ["ESP32-C3", "PCB design", "Optocouplers", "Wi-Fi"],
    role: "Solo: schematic, PCB, soldering, firmware",
    links: { repo: "" } // TODO
  },
  {
    id: "espnow",
    title: "ESP-NOW sensor network & door alarm",
    titleZh: "ESP-NOW 传感器网络与门磁报警",
    tag: "Wireless · Low power",
    category: "embedded",
    featured: true,
    image: "",
    summary: "Battery nodes read temperature, humidity, light and soil moisture and sleep most of the time. A door node detects when the door opens and the receiver sends an instant phone notification. Debugged real issues like the ADC2/Wi-Fi conflict and deep-sleep crashes.",
    stack: ["ESP32", "ESP-NOW", "Deep sleep", "NVS", "Telegram bot"],
    role: "Solo",
    links: { repo: "" } // TODO
  },
  {
    id: "robot-car",
    title: "Autonomous 4WD robot car",
    titleZh: "四驱自主避障小车",
    tag: "Politecnico di Torino · AI course",
    category: "robotics",
    featured: true,
    image: "assets/robot-car.jpg",
    summary: "Team project for an AI course: a 4WD Arduino car with an ultrasonic sensor that drives on its own and avoids obstacles.",
    stack: ["Arduino", "Ultrasonic sensor", "Motor control"],
    role: "Wrote the control and obstacle-avoidance code",
    links: { repo: "" } // TODO
  },
  {
    id: "homelab",
    title: "Two-site homelab",
    titleZh: "双站点家庭服务器",
    tag: "Ongoing",
    category: "systems",
    featured: true,
    image: "assets/homelab-dashboard.jpg", // screenshot della dashboard, ritagliata (niente domini/IP/preferiti)
    summary: "Proxmox servers in Italy and Romania linked by a VPN, with automatic cross-site backups and DNS failover. Frigate runs real-time object detection on cameras with a Coral TPU.",
    stack: ["Proxmox", "Docker", "WireGuard", "Frigate + Coral TPU"],
    role: "Solo",
    links: { repo: "" }
  },
  {
    id: "memory",
    title: "Memory game",
    tag: "Web",
    category: "software",
    image: "",
    summary: "Browser memory game with an online leaderboard.",
    stack: ["HTML", "CSS", "JavaScript"],
    links: { repo: "https://github.com/Cosmin702/Memory" }
  },
  {
    id: "recipes",
    title: "Recipe scraper + IBM watsonx",
    tag: "Python",
    category: "software",
    image: "",
    summary: "Collects recipes from the web and processes them with IBM watsonx.",
    stack: ["Python", "Web scraping", "IBM watsonx"],
    links: { repo: "" }
  },
  {
    id: "codicefiscale",
    title: "Italian tax-code calculator",
    tag: "C",
    category: "software",
    image: "",
    summary: "Computes the Italian codice fiscale from personal data.",
    stack: ["C"],
    links: { repo: "https://github.com/Cosmin702/codicefiscale" }
  },
  {
    id: "pc-bot",
    title: "Remote PC control bot",
    tag: "C# / .NET",
    category: "software",
    image: "",
    summary: "A Telegram bot talking to a Windows service to control my PC remotely.",
    stack: ["C#", ".NET", "Windows Services"],
    links: { repo: "" }
  }
];

// Modelli MakerWorld: aggiungi un oggetto per modello.
// image: link diretto alla foto oppure un file in assets/
const MAKERWORLD = [
  // { title: "Nome modello", url: "https://makerworld.com/en/models/...", image: "assets/mw-1.jpg", note: "breve descrizione" },
];
